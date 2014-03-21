#include "MapTool.h"
#include "wx/filedlg.h"
#include "wx/msgdlg.h"

#include "Map.h"

MapTool::MapTool(wxWindow* parent)
:MapFrameBase(parent)
{
    this->SetTitle(wxT("������Ե��ͼ����V1.0 - by С�Ե���"));
    this->SetIcon(wxICON(aaaa));
    this->SetSize(800, 600);
    this->Center();
}

MapTool::~MapTool()
{
    //dtor
}

void MapTool::OpenMap(wxCommandEvent& event)
{
    wxFileDialog filedlg(this,
                         wxT("��ѡ��һ����ͼ�ļ�"),
                         wxEmptyString,
                         wxEmptyString,
                         wxT("MAP�ļ�(*.map)|*.map"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if(filedlg.ShowModal() != wxID_OK) return;
    Map map;

    if(!map.ReadFile(filedlg.GetPath())) return;
    wxMessageBox(wxT("1"));
    m_MapPath->SetLabel(filedlg.GetPath());
    m_MapImg = map.getImage();
    wxMessageBox(wxT("2"));
    m_MapView->SetBitmap(wxBitmap(m_MapImg));
    m_WindowMapView->FitInside();
    wxMessageBox(wxT("3"));
}

void MapTool::SaveToPNG(wxCommandEvent& event)
{
    wxFileDialog filedlg(this,
                         wxT("��ΪPNG"),
                         wxEmptyString,
                         wxEmptyString,
                         wxT("MAP�ļ�(*.png)|*.png"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if(filedlg.ShowModal() != wxID_OK) return;
    m_MapImg.SaveFile(filedlg.GetPath(), wxBITMAP_TYPE_PNG);
}
