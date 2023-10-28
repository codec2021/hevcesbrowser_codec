#include "CommonInfoViewer.h"

#include <QStringList>
#include <QString>
#include <QSettings>
#include <QDebug>

#include <ConvToString.h>


CommonInfoViewer::CommonInfoViewer(QWidget *pwgt):
  QTableWidget(0, 4, pwgt)
{
  QStringList labels;
  labels << "Offset" << "Length" << "Nal Unit Type" << "Info";
  setHorizontalHeaderLabels(labels);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);

  QObject::connect(selectionModel(),
         SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
         this,
         SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));

  readCustomData();
}


void CommonInfoViewer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info *pInfo)
{
  NALUInfo nalUInfo;
  nalUInfo.m_pNALUnit = pNALUnit;
  nalUInfo.m_info = *pInfo;

  m_nalus.push_back(nalUInfo);

  int row = rowCount();
  insertRow(row);

  static int frameNum = 0;

  QString tmp = "";
  //QString offset = "0x"+ QString::number(pInfo -> m_position, 16) + " (" + QString::number(pInfo -> m_position) + ")";
  for (int i = 0; i < 8 - QString::number(pInfo -> m_position, 16).length(); i++)
  {
    tmp += "0";
  }
  QString offset = "0x"+ tmp + QString::number(pInfo -> m_position, 16);

  QTableWidgetItem *offset_item = new QTableWidgetItem(offset);
  //offset_item->setTextAlignment(Qt::AlignCenter);
  setItem(row, 0, offset_item);

  setItem(row, 1, new QTableWidgetItem(QString()));

  if(row > 0)
    item(row - 1, 1) -> setText(QString::number(pInfo -> m_position - m_nalus[m_nalus.size() - 2].m_info.m_position));

  setItem(row, 2, new QTableWidgetItem(QString((ConvToString::NALUnitType(pNALUnit -> m_nalHeader.type)).c_str())));

  using namespace HEVC;

  switch(pNALUnit -> m_nalHeader.type)
  {
    case NAL_VPS:
    {
      QTableWidgetItem *vps_item = new QTableWidgetItem("Video Parameter Set");
      vps_item->setTextColor(QColor("#FF8888"));
      setItem(row, 3, vps_item);
      std::shared_ptr<HEVC::VPS> pVPS = std::dynamic_pointer_cast<HEVC::VPS>(pNALUnit);
      using namespace HEVC;
      if(m_vpsMap.find(pVPS -> vps_video_parameter_set_id) == m_vpsMap.end() || !(*m_vpsMap[pVPS -> vps_video_parameter_set_id] == *pVPS))
      {
        m_vpsMap[pVPS -> vps_video_parameter_set_id] = pVPS;
        emit parameterSetsChanged(m_vpsMap, m_spsMap, m_ppsMap);
      }

      break;
    }

    case NAL_SPS:
    {
      QTableWidgetItem *sps_item = new QTableWidgetItem("Sequence Parameter Set");
      sps_item->setTextColor(QColor("#FF8888"));
      setItem(row, 3, sps_item);
      std::shared_ptr<HEVC::SPS> pSPS = std::dynamic_pointer_cast<HEVC::SPS>(pNALUnit);

      if(m_spsMap.find(pSPS -> sps_seq_parameter_set_id) == m_spsMap.end() || !(*m_spsMap[pSPS -> sps_seq_parameter_set_id] == *pSPS))
      {
        m_spsMap[pSPS -> sps_seq_parameter_set_id] = pSPS;
        emit parameterSetsChanged(m_vpsMap, m_spsMap, m_ppsMap);
      }


      break;
    }

    case NAL_PPS:
    {
      QTableWidgetItem *pps_item = new QTableWidgetItem("Picture Parameter Set");
      pps_item->setTextColor(QColor("#FF8888"));
      setItem(row, 3, pps_item);
      std::shared_ptr<HEVC::PPS> pPPS = std::dynamic_pointer_cast<HEVC::PPS>(pNALUnit);

      if(m_ppsMap.find(pPPS -> pps_pic_parameter_set_id) == m_ppsMap.end() || !(*m_ppsMap[pPPS -> pps_pic_parameter_set_id] == *pPPS))
      {
        m_ppsMap[pPPS -> pps_pic_parameter_set_id] = pPPS;
        emit parameterSetsChanged(m_vpsMap, m_spsMap, m_ppsMap);
      }

      break;
    }


    case NAL_IDR_W_RADL:
    case NAL_IDR_N_LP:
    {
      QTableWidgetItem *item = new QTableWidgetItem("IDR Slice #" + QString::number(frameNum));
      item->setTextColor(QColor("red"));
      setItem(row, 3, item);
      frameNum++;
      break;
    }


    case NAL_TRAIL_R:
    case NAL_TSA_R:
    case NAL_STSA_R:
    case NAL_RADL_R:
    case NAL_RASL_R:
    case NAL_TRAIL_N:
    case NAL_TSA_N:
    case NAL_STSA_N:
    case NAL_RADL_N:
    case NAL_RASL_N:
    case NAL_BLA_W_LP:
    case NAL_BLA_W_RADL:
    case NAL_BLA_N_LP:
    case NAL_CRA_NUT:
    {
      std::shared_ptr<HEVC::Slice> pSlice = std::dynamic_pointer_cast<HEVC::Slice>(pNALUnit);

      if(pSlice -> dependent_slice_segment_flag)
        setItem(row, 3, new QTableWidgetItem("Dependent Slice"));
      else
      {
        switch(pSlice -> slice_type)
        {
          case HEVC::Slice::B_SLICE:
          {
            QTableWidgetItem *B_item = new QTableWidgetItem("B Slice #" + QString::number(frameNum));
            if (pNALUnit->m_nalHeader.type == NAL_TSA_N)
            {
              B_item->setTextColor(QColor("#FF6EB4"));
            }
            else if (pNALUnit->m_nalHeader.type == NAL_TSA_R)
            {
              B_item->setTextColor(QColor("#FF83FA"));
            }
            else if (pNALUnit->m_nalHeader.type == NAL_TRAIL_N)
            {
              B_item->setTextColor(QColor("#E066FF"));
            }
            else if (pNALUnit->m_nalHeader.type == NAL_TRAIL_R)
            {
              B_item->setTextColor(QColor("#EE30A7"));
            }
            else
            {
              B_item->setTextColor(QColor("#FFB90F"));
            }
            setItem(row, 3, B_item);
            break;
          }
          case HEVC::Slice::P_SLICE:
          {
            QTableWidgetItem *P_item = new QTableWidgetItem("P Slice #" + QString::number(frameNum));
            P_item->setTextColor(QColor("#0000FF"));
            setItem(row, 3, P_item);
            break;
          }
          case HEVC::Slice::I_SLICE:
          {
            QTableWidgetItem *I_item = new QTableWidgetItem("I Slice #" + QString::number(frameNum));
            I_item->setTextColor(QColor("#CD9B1D"));
            setItem(row, 3, I_item);
            break;
          }
        };
      }
      frameNum++;
      break;
    }

    case NAL_AUD:
    {
      setItem(row, 3, new QTableWidgetItem("Access unit delimiter"));
      break;
    }

    case NAL_EOS_NUT:
    {
      setItem(row, 3, new QTableWidgetItem("End of sequence"));
      break;
    }

    case NAL_EOB_NUT:
    {
      setItem(row, 3, new QTableWidgetItem("End of bitstream"));
      break;
    }

    case NAL_FD_NUT:
    {
      setItem(row, 3, new QTableWidgetItem("Filler data"));
      break;
    }

    case NAL_SEI_PREFIX:
    case NAL_SEI_SUFFIX:
    {
      QTableWidgetItem *sei_item = new QTableWidgetItem("Supplemental Enhancement Information");
      sei_item->setTextColor(QColor("#BCEE68"));
      setItem(row, 3, sei_item);
      break;
    }

    default:
      setItem(row, 3, new QTableWidgetItem(""));
  };

    setRowHeight(row, 4);
}



void CommonInfoViewer::onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
  QModelIndexList idxs = selected.indexes();

  if(!idxs.empty())
    emit naluSelected(m_nalus[idxs.begin() -> row()].m_pNALUnit, m_nalus[idxs.begin() -> row()].m_info);
}


void CommonInfoViewer::clear()
{
  setRowCount(0);

  m_nalus.clear();
}


void CommonInfoViewer::saveCustomData()
{
  QSettings settings("HEVCESBrowser", "HEVCESBrowser");
  settings.setValue("CommonInfoViewer/0000Width", columnWidth(0));
  settings.setValue("CommonInfoViewer/1Width", columnWidth(1));
  settings.setValue("CommonInfoViewer/2Width", columnWidth(2));
  settings.setValue("CommonInfoViewer/3Width", columnWidth(3));
}


void CommonInfoViewer::readCustomData()
{
  QSettings settings("HEVCESBrowser", "HEVCESBrowser");

  if(settings.contains("CommonInfoViewer/0000Width"))
    setColumnWidth(0, settings.value("CommonInfoViewer/0000Width").toInt());

  if(settings.contains("CommonInfoViewer/1Width"))
    setColumnWidth(1, settings.value("CommonInfoViewer/1Width").toInt());

  if(settings.contains("CommonInfoViewer/2Width"))
    setColumnWidth(2, settings.value("CommonInfoViewer/2Width").toInt());

  if(settings.contains("CommonInfoViewer/3Width"))
    setColumnWidth(3, settings.value("CommonInfoViewer/3Width").toInt());
}
