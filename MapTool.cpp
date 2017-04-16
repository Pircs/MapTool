#include "MapTool.h"
#include "YesNoAllDialog.h"
#include "Settings.h"
#include "FpsDialog.h"
#include "wx/msgdlg.h"
#include "wx/dcmemory.h"

#include "wx/accel.h"
#include "wx/utils.h"

#define MAX_COMMAND 1000 //Max undo redo command

using namespace std;

wxPoint MapTool::INVALID_TILE_POSITION = wxPoint(-1, -1);

/** \brief Disable timer when constructing, enable timer when destructing.
 *
 */
class TimerDisabler
{
public:
	TimerDisabler(MapTool *mapTool)
	{
		m_mapTool = mapTool;
		m_mapTool->DisableTimer();
	}
	~TimerDisabler()
	{
		m_mapTool->EnableTimer();
	}
private:
	MapTool *m_mapTool;
};

BEGIN_EVENT_TABLE(MapTool, MapFrameBase)
    EVT_MENU(ID_MAPUP, MapTool::OnMapUp)
    EVT_MENU(ID_MAPDOWN, MapTool::OnMapDown)
    EVT_MENU(ID_MAPLEFT, MapTool::OnMapLeft)
    EVT_MENU(ID_MAPRIGHT, MapTool::OnMapRight)
    EVT_MENU(ID_DIRECTION, MapTool::OnCharacterDirection)
    EVT_MENU(ID_SHOWNPCKEY, MapTool::OnShowNpcCheckKey)
    EVT_MENU(ID_SHOWOBJKEY, MapTool::OnShowObjCheckKey)
    EVT_MENU(ID_ATTRIBUTELISTITEM, MapTool::AttributeListItem)
    EVT_MENU(ID_DELETELISTITEM, MapTool::DeleteListItem)
    EVT_MENU(ID_BATEDITLISTITEM, MapTool::BatEditListItem)
    EVT_CHAR_HOOK(MapTool::FrameOnChar)
END_EVENT_TABLE()


MapTool::MapTool(wxWindow* parent)
    : MapFrameBase(parent)
    , m_commandProcessor(MAX_COMMAND)
{
    m_ViewBeginx = m_ViewBeginy = 0;
    m_CurTileX = m_CurTileY = 0;
    m_PlaceNpcData.Dir = 0;
    m_PlaceNpcData.NpcStand = new AsfDecode;
    m_PlaceObjData.Dir = 0;
    m_PlaceObjData.ObjCommon = new AsfDecode;
    m_isPlaceMode = true;
    m_isDeleteMode = false;
    m_isEditAttribute = false;
    m_isMoveMode = false;
    m_NpcAsfImgList = new AsfImgList;
    m_ObjAsfImgList = new AsfImgList;
    m_MoveNpcItem = NULL;
    m_MoveObjItem = NULL;
    m_selectedNpcItemByRightUp = NULL;
    m_selectedObjItemByRightUp = NULL;
    m_isNpc = true;
    m_isObj = false;
    m_placeModeNotDraw = false;
    m_npcItemEdit = NULL;
    m_isEditFixPos = false;
    m_popupMenuShowed = false;

    exepath = wxStandardPaths::Get().GetExecutablePath();
    exepath = wxFileName::FileName(exepath).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    m_NpcObjPath = exepath + wxT("ini\\save\\");

    m_ToolBarEdit->ToggleTool(ID_TOOLPLACE, true);
    m_ToolBarEdit->ToggleTool(ID_NPCMODE, true);
    m_ToolBarEdit->ToggleTool(ID_SHOWNPC, true);
    m_ToolBarEdit->ToggleTool(ID_SHOWOBJ, true);

    this->SetTitle(wxT("������Ե��ͼ����V2.8.1 - by С�Ե���  2017.4.16"));
    this->SetIcon(wxICON(aaaa));
    this->SetSize(800, 600);
    this->Center();

    wxAcceleratorEntry ace[13];
    ace[0].Set(wxACCEL_NORMAL, WXK_UP, ID_MAPUP);
    ace[1].Set(wxACCEL_NORMAL, WXK_DOWN, ID_MAPDOWN);
    ace[2].Set(wxACCEL_NORMAL, WXK_LEFT, ID_MAPLEFT);
    ace[3].Set(wxACCEL_NORMAL, WXK_RIGHT, ID_MAPRIGHT);
    ace[4].Set(wxACCEL_NORMAL, WXK_SPACE, ID_DIRECTION);
    ace[5].Set(wxACCEL_NORMAL, WXK_F2, ID_NPCMODE);
    ace[6].Set(wxACCEL_NORMAL, WXK_F3, ID_OBJMODE);
    ace[7].Set(wxACCEL_NORMAL, 'P', ID_TOOLPLACE);
    ace[8].Set(wxACCEL_NORMAL, 'D', ID_TOOLDELETE);
    ace[9].Set(wxACCEL_NORMAL, 'A', ID_EDITATTRIBUTE);
    ace[10].Set(wxACCEL_NORMAL, 'M', ID_TOOLMOVE);
    ace[11].Set(wxACCEL_NORMAL, 'N', ID_SHOWNPCKEY);
    ace[12].Set(wxACCEL_NORMAL, 'O', ID_SHOWOBJKEY);
    wxAcceleratorTable act(13, ace);
    SetAcceleratorTable(act);

    //List config
    m_ListData->Show(false);
    m_npcListCtrl->AppendColumn(wxT("���"), wxLIST_FORMAT_LEFT, 50);
    m_npcListCtrl->AppendColumn(wxT("X"), wxLIST_FORMAT_LEFT, 50);
    m_npcListCtrl->AppendColumn(wxT("Y"), wxLIST_FORMAT_LEFT, 50);
    m_objListCtrl->AppendColumn(wxT("���"), wxLIST_FORMAT_LEFT, 50);
    m_objListCtrl->AppendColumn(wxT("X"), wxLIST_FORMAT_LEFT, 50);
    m_objListCtrl->AppendColumn(wxT("Y"), wxLIST_FORMAT_LEFT, 50);

    //timer
    m_timer.SetOwner(this);
    this->Connect(wxEVT_TIMER, wxTimerEventHandler(MapTool::OnTimer), NULL, this);
    EnableTimer();

    //Batch selection
    m_batchSelectionBeginTile = m_batchSelectionEndTile = INVALID_TILE_POSITION;
    m_tilePositionOffsetNpc = m_tilePositionOffsetObj = wxPoint(0, 0);

    m_isBatch = false;

    m_commandProcessor.SetEditMenu(m_menuEdit);
    MTC_Init(this, exepath, &m_NpcList, &m_ObjList);
}

MapTool::~MapTool()
{
    m_timer.Stop();
    this->Disconnect(wxEVT_TIMER, wxTimerEventHandler(MapTool::OnTimer), NULL, this);
    if(m_NpcAsfImgList != NULL)
    {
        FreeAsfImgList(m_NpcAsfImgList);
        delete m_NpcAsfImgList;
    }
    if(m_ObjAsfImgList != NULL)
    {
        FreeAsfImgList(m_ObjAsfImgList);
        delete m_ObjAsfImgList;
    }
    if(m_PlaceNpcData.NpcStand != NULL) delete m_PlaceNpcData.NpcStand;
    if(m_PlaceObjData.ObjCommon != NULL) delete m_PlaceObjData.ObjCommon;
    if(m_npcItemEdit) delete m_npcItemEdit;
}
void MapTool::FrameOnChar( wxKeyEvent& event )
{
    int keyCode = event.GetKeyCode();

    if(m_isEditFixPos)
    {
        if(keyCode == WXK_RETURN)EndFixPosEdit();
        else if(keyCode == WXK_ESCAPE)
        {
            if(m_fixPosPoints.size() > 1)
                m_fixPosPoints.pop_back();
            RedrawMapView();
        }
    }

    event.Skip();
}
void MapTool::OnClose(wxCloseEvent& event)
{
    if(m_MapFileName.IsEmpty())
    {
        Exit();
    }
    else
    {
        int ret = wxMessageBox(wxT("�˳���"), wxT("ȷ��"), wxYES_NO | wxCENTER | wxICON_QUESTION);
        if(ret == wxYES) Exit();
    }
}

void MapTool::Exit()
{
	map.ShutDownClient();
	m_timer.Stop();
	Destroy();
}

void MapTool::OpenMap(wxCommandEvent& event)
{
	wxWindowDisabler disableUI();
    wxFileDialog filedlg(this,
                         wxT("��ѡ��һ����ͼ�ļ�"),
                         exepath + wxT("map\\"),
                         m_MapFileName,
                         wxT("MAP�ļ�(*.map, *.tmx)|*.map;*.tmx"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if(filedlg.ShowModal() != wxID_OK) return;

    if(!map.ReadFile(filedlg.GetPath())) return;
    m_MapFileName = filedlg.GetFilename();
    this->SetTitle(wxString::Format(wxFormatString(wxT("%s      %ld �� %ld - %ld �� %ld")),
                                    m_MapFileName,
                                    map.getCol(),
                                    map.getRow(),
                                    map.getPixelWidth(),
                                    map.getPixelHeight()));

    //clear npcs
    m_NpcList.Clear();
    m_ObjList.Clear();
    FreeAsfImgList(m_NpcAsfImgList);
    FreeAsfImgList(m_ObjAsfImgList);

    m_commandProcessor.ClearCommands();
    m_menuEdit->Enable(wxID_UNDO, false);
    m_menuEdit->Enable(wxID_REDO, false);
	m_menuEdit->SetLabel(wxID_UNDO, wxT("����\tCTRL+Z"));
	m_menuEdit->SetLabel(wxID_REDO, wxT("����\tCTRL+Y"));

    //Show view
    m_ListData->Show(true);
    m_panelList->Layout();
    RefreshNpcList();
    RefreshObjList();

    m_MapView->Refresh(true);
    ReadMap();
    RedrawMapView();
}

void MapTool::SaveToPNG(wxCommandEvent& event)
{
    wxFileDialog filedlg(this,
                         wxT("��ΪPNG"),
                         wxEmptyString,
                         wxEmptyString,
                         wxT("PNG�ļ�(*.png)|*.png"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if(filedlg.ShowModal() != wxID_OK) return;
    wxImage *img = ReadMap(true);
    bool success = false;
    if(img != NULL)
    {
        if(IsDrawObjsNpcs())
        {
            wxBitmap bmp(*img);
            wxMemoryDC dc(bmp);
            if(dc.IsOk())
            {
                DrawObjsNpcs(dc, false);
                dc.SelectObject(wxNullBitmap);
                success = bmp.SaveFile(filedlg.GetPath(), wxBITMAP_TYPE_PNG);
            }
            else
                success = false;
        }
        else
        {
            success = img->SaveFile(filedlg.GetPath(), wxBITMAP_TYPE_PNG);
        }
    }

    if(success)
        wxMessageBox(wxT("���"), wxT("��Ϣ"));
    else
        wxMessageBox(wxT("ʧ��"), wxT("����"), wxOK|wxCENTER|wxICON_ERROR);

    if(img != NULL) delete img;
}

wxImage* MapTool::ReadMap(bool getImg)
{
    unsigned char layer = 0;
    if(m_Layer1->IsChecked()) layer |= Map::LAYER1;
    if(m_Layer2->IsChecked()) layer |= Map::LAYER2;
    if(m_Layer3->IsChecked()) layer |= Map::LAYER3;
    if(m_Barrer->IsChecked()) layer |= Map::BARRER;
    if(m_Trap->IsChecked())   layer |= Map::TRAP;

    wxImage *img = map.getImage(layer, m_LayerTransparent->IsChecked());
    m_MapBitmap = wxBitmap(*img);
    if(getImg) return img;
    else
    {
        delete img;
        return NULL;
    }
}

void MapTool::RedrawNow()
{
    m_MapView->Refresh(false);
    m_MapView->Update();
    m_MapControl->Refresh(false);
    m_MapControl->Update();
}


void MapTool::RedrawMapView()
{
    if(m_placeModeNotDraw)
    {
        m_placeModeNotDraw = false;
    }
}

void MapTool::OnTimer(wxTimerEvent& event)
{
    RedrawNow();
}

void MapTool::OnSetFps(wxCommandEvent& event)
{
    FpsDialog dialog(this);
    dialog.SetFpsMilliseconds(Settings::TheSetting.GetFpsMilliseconds());
    if(dialog.ShowModal() == wxID_OK)
    {
        int millsecond = 1000/dialog.GetFps();
        m_timer.Stop();
        m_timer.Start(millsecond);
        Settings::TheSetting.SetFpsMilliseconds(millsecond);
    }
}

void MapTool::OnSetTmxHelperPort(wxCommandEvent& event)
{
	SetTmxHelperPortDialog dialog(this);
	dialog.ShowModal();
}

void MapTool::DisableTimer()
{
    m_timer.Stop();
}

void MapTool::EnableTimer()
{
    m_timer.Start(Settings::TheSetting.GetFpsMilliseconds());
}

void MapTool::ReNewNpcAsf(NpcItem* item)
{
	FindAndBufferAsf(exepath, item->NpcIni, wxT("[Stand]"), &(item->NpcStand), m_NpcAsfImgList);
}

void MapTool::ReNewObjAsf(ObjItem* item)
{
	FindAndBufferAsf(exepath, item->ObjFile, wxT("[Common]"), &(item->ObjCommon), m_ObjAsfImgList);
}

void MapTool::OnMapDraw( wxPaintEvent& event )
{
    if(!m_MapBitmap.IsOk()) return;

    bool isInMultyPaste = IsInMultyPaste();

    int viewWidth, viewHeight, bmpWidth, bmpHeight;
    m_MapView->GetClientSize(&viewWidth, &viewHeight);
    bmpWidth = m_MapBitmap.GetWidth();
    bmpHeight = m_MapBitmap.GetHeight();

    wxImage img(viewWidth, viewHeight, true);
    if(m_LayerTransparent->IsChecked()) img.SetAlpha();
    wxBitmap bufbitmap(img);
    img.Destroy();
    wxBufferedPaintDC dc(m_MapView, bufbitmap);
    wxMemoryDC memdc;

    if(m_ViewBeginx + viewWidth > bmpWidth) m_ViewBeginx = bmpWidth - viewWidth;
    if(m_ViewBeginy + viewHeight > bmpHeight) m_ViewBeginy = bmpHeight - viewHeight;
    if(m_ViewBeginx < 0) m_ViewBeginx = 0;
    if(m_ViewBeginy < 0) m_ViewBeginy = 0;

    //check whether bmp size is small than view
    if(m_ViewBeginx + viewWidth > bmpWidth) viewWidth = bmpWidth;
    if(m_ViewBeginy + viewHeight > bmpHeight) viewHeight = bmpHeight;

    memdc.SelectObject(m_MapBitmap);

    //Draw Map
    dc.Blit(0,
            0,
            viewWidth,
            viewHeight,
            &memdc,
            m_ViewBeginx,
            m_ViewBeginy,
            wxCOPY,
            m_LayerTransparent->IsChecked());

    if(m_isEditFixPos)
    {
        //Do nothing
    }
    else if(m_isPlaceMode)//Draw PlaceMode
    {
        if(m_placeModeNotDraw ||
			IsInSelectingItem() ||
			isInMultyPaste)
		{
			//do nothing
		}
		else
        {
            if(m_isNpc)
				DrawTile(m_CurTileX, m_CurTileY, dc, &m_PlaceNpcData);
			else if(m_isObj)
				DrawTile(m_CurTileX, m_CurTileY, dc, NULL, &m_PlaceObjData);
        }
    }

    DrawObjsNpcs(dc);
    if(m_isPlaceMode || IsInSelectingItem() || isInMultyPaste)
	{
		//do nothing
	}
	else
	{
		DrawObjsNpcsPosition(dc);
	}


    NpcItem *curItem = m_NpcList.GetItem(m_CurTileX, m_CurTileY);
    if(!m_isPlaceMode &&
            !m_isEditFixPos &&
            curItem && !curItem->FixedPos.IsEmpty())
    {
        std::list<wxPoint> *posList = NpcItemEditDialog::ToFixPosList(curItem->FixedPos);
        if(posList)
        {
            dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxGREEN)));
            size_t count = posList->size();
            wxPoint pos[count];
            int index = 0;
            for(std::list<wxPoint>::iterator it = posList->begin();
                    it != posList->end(); it++)
            {
                pos[index++] = wxPoint(it->x, it->y);
            }
            delete posList;

            dc.DrawLines(count, pos, -m_ViewBeginx, -m_ViewBeginy);
        }
    }

    //Draw npc or obj list selection
    dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColor(255, 128, 0), 2)));
    DrawListSelection(dc);

    //Draw reactangle current positon under mouse
    if(map.IsTileBarrer(m_CurTileX, m_CurTileY))
        dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxRED, 2)));
    else if(map.IsTileBarrerTrans(m_CurTileX, m_CurTileY))
        dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxBLUE, 2)));
    else if(curItem ||
            m_ObjList.HasItem(m_CurTileX, m_CurTileY))
        dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxYELLOW, 2)));
    else
        dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxGREEN)));
    //Don't draw curren tile position when item selected at this position when in selecting item.
    if((IsInSelectingItem() && IsItemSelectedAtTile(m_CurTileX, m_CurTileY)) || isInMultyPaste)
    {
        //do nothing
    }
    else
	{
		DrawRectangle(m_CurTileX, m_CurTileY, dc);
	}

    if(m_isEditFixPos)
    {
        DrawEditFixPos(dc);
    }
}

void MapTool::DrawRectangle(long col, long row, wxDC &dc, bool currentView)
{
    int recposx, recposy;
    if(map.GetPixelPosition(col, row, &recposx, &recposy))
    {
        wxPoint point[5];
        point[0] = wxPoint(32, 0);
        point[1] = wxPoint(64, 16);
        point[2] = wxPoint(32, 32);
        point[3] = wxPoint(0, 16);
        point[4] = wxPoint(32, 0);
        if(currentView)
        {
            recposx -= m_ViewBeginx;
            recposy -= m_ViewBeginy;
        }
        dc.DrawLines(5, point, recposx, recposy);
    }
}
void MapTool::DrawRectangle(wxPoint tileStart, wxPoint tileEnd, wxDC &dc, bool currentView)
{
	int sX, sY, eX, eY;
    if(map.GetPixelPosition(tileStart.x, tileStart.y, &sX, &sY) &&
		map.GetPixelPosition(tileEnd.x, tileEnd.y, &eX, &eY))
	{
		int x = std::min(sX, eX);
		int y = std::min(sY, eY);
		int w = std::abs(sX - eX);
		int h = std::abs(sY - eY);
		if(currentView)
		{
			x -= m_ViewBeginx;
			y -= m_ViewBeginy;
		}
		dc.DrawRectangle(x,y,w,h);
	}
}
void MapTool::DrawTile(long col, long row, wxDC &dc, NpcItem *npcitem, ObjItem *objitem, bool currentView)
{
    if(npcitem == NULL && objitem == NULL) return;

    int recposx, recposy;
    if(!map.GetPixelPosition(col, row, &recposx, &recposy)) return;

    long tDrawX = 0, tDrawY = 0, tOffX = 0, tOffY = 0;
    wxBitmap tbmp;
    int tWidth = 0, tHeight = 0;
    if(npcitem != NULL)
    {
        if(npcitem->NpcStand)
        {
            tbmp = npcitem->NpcStand->GetDirectionBitmapFromBufferdData(npcitem->Dir);
            tWidth = tbmp.GetWidth();
            tHeight = tbmp.GetHeight();
            tOffX = npcitem->NpcStand->GetLeft();
            tOffY = npcitem->NpcStand->GetBottom();

            //tDrawX = recposx + 33 - tOffX - m_ViewBeginx;
            //tDrawY = recposy + 58 - tOffY + (32 - tHeight) - m_ViewBeginy;

            //Tile beg at tile middle(32,16)
            tDrawX = recposx + 32 - tOffX;
            tDrawY = recposy + 16 - tOffY;
        }
    }
    else if(objitem != NULL)
    {
        if(objitem->ObjCommon)
        {
            tbmp = objitem->ObjCommon->GetDirectionBitmapFromBufferdData(objitem->Dir, objitem->Frame);
            tWidth = tbmp.GetWidth();
            tHeight = tbmp.GetHeight();
            tOffX = objitem->ObjCommon->GetLeft();
            tOffY = objitem->ObjCommon->GetBottom();

            //Tile beg at tile middle(32,16)
            tDrawX = recposx + 32 - tOffX + objitem->GetOffX();
            tDrawY = recposy + 16 - tOffY + objitem->GetOffY();
        }
    }

    if(currentView)
    {
        tDrawX -= m_ViewBeginx;
        tDrawY -= m_ViewBeginy;
    }

    if(tbmp.IsOk())
    {
        wxMemoryDC memdc;
        memdc.SelectObject(tbmp);

        dc.Blit(tDrawX,
                tDrawY,
                tWidth,
                tHeight,
                &memdc,
                0,
                0,
                wxCOPY,
                true);
    }
}

void MapTool::DrawEditFixPos(wxDC& dc)
{
    size_t size = m_fixPosPoints.size() + 1;
    wxPoint point[size];
    typedef std::list<wxPoint>::iterator Itor;
    int i = 0;
    for(Itor it = m_fixPosPoints.begin();
            it != m_fixPosPoints.end(); it++, i++)
    {
        point[i] = *it;
    }
    int x, y;
    if(map.GetTileCenterPixelPosition(m_CurTileX, m_CurTileY, &x, &y))
    {
        point[size - 1] = wxPoint(x, y);
        dc.DrawLines(size, point, -m_ViewBeginx, -m_ViewBeginy);
    }
}

void MapTool::DrawListSelection(wxDC &dc)
{
    //When not in selecting, not draw
    if(!IsInSelectingItem()) return;

    if(m_isNpc)
    {
        std::vector<long> items = GetAllSelectedItems(m_npcListCtrl);

        for(std::vector<long>::iterator it = items.begin();
                it != items.end();
                it++)
        {
            NpcItem *item = m_NpcList.GetItem((int)*it);
            if(item)
            {
                DrawRectangle(item->MapX, item->MapY, dc);
            }
        }
    }
    else if(m_isObj)
    {
        std::vector<long> items = GetAllSelectedItems(m_objListCtrl);
        //Draw when multiple item selected

        for(std::vector<long>::iterator it = items.begin();
                it != items.end();
                it++)
        {
            ObjItem *item = m_ObjList.GetItem((int)*it);
            if(item)
            {
                DrawRectangle(item->MapX, item->MapY, dc);
            }
        }
    }

    if(m_batchSelectionBeginTile != INVALID_TILE_POSITION && m_batchSelectionEndTile != INVALID_TILE_POSITION)
	{
		dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColor(100, 0, 255), 2)));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		DrawRectangle(m_batchSelectionBeginTile, m_batchSelectionEndTile, dc);
	}
}

void MapTool::CorrectFixedPos(NpcItem* item)
{
    if(item && item->FixedPos.length() == 128)
    {
        wxString tmp = NpcItemEditDialog::PrintPosition(item->MapX) +
                       NpcItemEditDialog::PrintPosition(item->MapY);
        item->FixedPos = tmp + item->FixedPos.Mid(16);
    }
}

bool MapTool::IsDrawObjsNpcs()
{
    return (
               m_ToolBarEdit->GetToolState(ID_SHOWNPC) ||
               m_ToolBarEdit->GetToolState(ID_SHOWOBJ)
           ) &&
           (
               m_NpcList.getCounts() ||
               m_ObjList.getCounts()
           );
}

void MapTool::DrawObjsNpcs(wxDC &dc, bool currentView)
{
	NpcItem *npcitem = nullptr;
	ObjItem *objitem = nullptr;
    if(m_ToolBarEdit->GetToolState(ID_SHOWNPC))
    {
        NpcList::iterator end = m_NpcList.end();
        for(NpcList::iterator it = m_NpcList.begin(); it != end; it++)
        {
            npcitem = *it;
            if(npcitem == NULL) continue;
            DrawTile(npcitem->MapX, npcitem->MapY, dc, npcitem, NULL, currentView);
        }
    }

    if(m_ToolBarEdit->GetToolState(ID_SHOWOBJ))
    {
        ObjList::iterator end = m_ObjList.end();
        for(ObjList::iterator it = m_ObjList.begin(); it != end; it++)
        {
            objitem = *it;
            if(objitem == NULL) continue;
            DrawTile(objitem->MapX, objitem->MapY, dc, NULL, objitem, currentView);
        }
    }

    if(m_isNpc && m_npcsToPasted.getCounts() > 0)
	{
		for(NpcList::iterator it = m_npcsToPasted.begin(); it != m_npcsToPasted.end(); it++)
		{
			npcitem = *it;
			if(npcitem == NULL) continue;
			npcitem->MapX = npcitem->MapX - m_tilePositionOffsetNpc.x + m_CurTileX;
			npcitem->MapY = npcitem->MapY - m_tilePositionOffsetNpc.y + m_CurTileY;
			DrawTile(npcitem->MapX, npcitem->MapY, dc, npcitem, NULL, currentView);
			if(map.IsObstacleForCharacter(npcitem->MapX, npcitem->MapY))
			{
				//At obstacle tile
				dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(255, 0, 0), 2)));
				DrawRectangle(npcitem->MapX, npcitem->MapY, dc, currentView);
			}
			if(m_NpcList.GetItem(npcitem->MapX, npcitem->MapY))
			{
				//At the same tile
				dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(255, 255, 0), 2, wxPENSTYLE_DOT)));
				DrawRectangle(npcitem->MapX, npcitem->MapY, dc, currentView);
			}
			npcitem->MapX = npcitem->MapX + m_tilePositionOffsetNpc.x - m_CurTileX;
			npcitem->MapY = npcitem->MapY + m_tilePositionOffsetNpc.y - m_CurTileY;
		}
	}

	if(m_isObj && m_objsToPasted.getCounts() > 0)
	{
		dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(255, 0, 0), 2)));
		for(ObjList::iterator it = m_objsToPasted.begin(); it != m_objsToPasted.end(); it++)
		{
			objitem = *it;
			if(objitem == NULL) continue;
			objitem->MapX = objitem->MapX - m_tilePositionOffsetObj.x + m_CurTileX;
			objitem->MapY = objitem->MapY - m_tilePositionOffsetObj.y + m_CurTileY;
			DrawTile(objitem->MapX, objitem->MapY, dc, NULL, objitem, currentView);
			if(map.IsObstacleForCharacter(objitem->MapX, objitem->MapY))
			{
				//At obstacle tile
				dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(255, 0, 0), 2)));
				DrawRectangle(objitem->MapX, objitem->MapY, dc, currentView);
			}
			if(m_ObjList.GetItem(objitem->MapX, objitem->MapY))
			{
				//At the same tile
				dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(255, 255, 0), 2, wxPENSTYLE_DOT)));
				DrawRectangle(objitem->MapX, objitem->MapY, dc, currentView);
			}
			objitem->MapX = objitem->MapX + m_tilePositionOffsetObj.x - m_CurTileX;
			objitem->MapY = objitem->MapY + m_tilePositionOffsetObj.y - m_CurTileY;
		}
	}
}
void MapTool::DrawObjsNpcsPosition(wxDC &dc, bool currentView)
{
    if(m_isNpc && m_ToolBarEdit->GetToolState(ID_SHOWNPC))
    {
        dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(0, 255, 255), 2)));
        DrawNpcPosition(dc, currentView);
    }
    if(m_isObj && m_ToolBarEdit->GetToolState(ID_SHOWOBJ))
    {
        dc.SetPen(*(wxThePenList->FindOrCreatePen(wxColour(147, 244, 0), 2)));
        DrawObjPostion(dc, currentView);
    }
}

void MapTool::DrawNpcPosition(wxDC &dc, bool currentView)
{
    int counts = m_NpcList.getCounts();
    NpcItem *npcitem;
    for(int i = 0; i < counts; i++)
    {
        npcitem = m_NpcList.GetItem(i);
        if(npcitem == NULL) continue;
        DrawRectangle(npcitem->MapX, npcitem->MapY, dc, currentView);
    }
}

void MapTool::DrawObjPostion(wxDC &dc, bool currentView)
{
    int counts = m_ObjList.getCounts();
    ObjItem *objitem;
    for(int j = 0; j < counts; j++)
    {
        objitem = m_ObjList.GetItem(j);
        if(objitem == NULL) continue;
        DrawRectangle(objitem->MapX, objitem->MapY, dc, currentView);
    }
}

void MapTool::StartFixPosEdit(NpcItem* npcitem)
{
    m_isEditFixPos = true;
    m_placeModeNotDraw = false;
    m_fixPosEditItem = npcitem;
    m_fixPosPoints.clear();
    int x,y;
    map.GetTileCenterPixelPosition(npcitem->MapX, npcitem->MapY, &x, &y);
    m_fixPosPoints.push_back(wxPoint(x, y));
}

void MapTool::EndFixPosEdit(bool isCancle)
{
    m_isEditFixPos = false;
    if(!isCancle)m_npcItemEdit->SetFixPos(NpcItemEditDialog::ToFixPosString(m_fixPosPoints));
    m_fixPosPoints.clear();
    NpcItemEditShowModle(m_npcItemEdit, m_fixPosEditItem);
}

void MapTool::OnLayerTransparent( wxCommandEvent& event )
{
    ReadMap();
    m_MapView->Refresh(true);
    RedrawMapView();
}
void MapTool::OnLayer1( wxCommandEvent& event )
{
    ReadMap();
    RedrawMapView();
}
void MapTool::OnLayer2( wxCommandEvent& event )
{
    ReadMap();
    RedrawMapView();
}
void MapTool::OnLayer3( wxCommandEvent& event )
{
    ReadMap();
    RedrawMapView();
}
void MapTool::OnTrap( wxCommandEvent& event )
{
    ReadMap();
    RedrawMapView();
}
void MapTool::OnBarrer( wxCommandEvent& event )
{
    ReadMap();
    RedrawMapView();
}

void MapTool::OnMapUp( wxCommandEvent& event )
{
    m_ViewBeginy -= 16;
    CheckMapViewBeginPosition();
    RedrawMapView();
}
void MapTool::OnMapDown( wxCommandEvent& event )
{
    m_ViewBeginy += 16;
    CheckMapViewBeginPosition();
    RedrawMapView();
}
void MapTool::OnMapLeft( wxCommandEvent& event )
{
    m_ViewBeginx -= 64;
    CheckMapViewBeginPosition();
    RedrawMapView();
}
void MapTool::OnMapRight( wxCommandEvent& event )
{
    m_ViewBeginx += 64;
    CheckMapViewBeginPosition();
    RedrawMapView();
}
void MapTool::OnMapViewMouseLeftDown( wxMouseEvent& event )
{
	if(m_isNpc && m_npcsToPasted.getCounts() > 0)
	{
		//Do nothing
	}
	else if(m_isObj && m_objsToPasted.getCounts() > 0)
	{
		//Do nothing
	}
    else if(IsInSelectingItem())
    {
        //Selection mode, add selection
        if(m_isNpc)
        {
            ToggleNpcSelection(m_CurTileX, m_CurTileY);
        }
        else if(m_isObj)
        {
        	ToggleObjSelection(m_CurTileX, m_CurTileY);
        }
        m_batchSelectionBeginTile = m_batchSelectionEndTile = wxPoint(m_CurTileX, m_CurTileY);
    }
    else if(m_isEditFixPos)
    {
        //Do nothing
    }
    else if(m_popupMenuShowed)
    {
        // pou up menu of map view is show
        // ingnore this event once
        m_popupMenuShowed = false;
    }
    else if(m_isPlaceMode)
    {
        if(m_isNpc && !m_NpcIniFilePath.IsEmpty())
        {
            NpcItem *npcitem = new NpcItem;
            if(!ReadIni(exepath, m_NpcIniFilePath,  npcitem, NULL, m_NpcAsfImgList))
            {
                delete npcitem;
                return;
            }
            npcitem->MapX = m_CurTileX;
            npcitem->MapY = m_CurTileY;
            npcitem->Dir = m_PlaceNpcData.Dir;
            if(!AddItem(npcitem))
            {
                delete npcitem;
            }
        }
        else if(m_isObj && !m_ObjIniFilePath.IsEmpty())
        {
            ObjItem *objitem = new ObjItem;
            if(!ReadIni(exepath, m_ObjIniFilePath, NULL, objitem, m_ObjAsfImgList))
            {
                delete objitem;
                return;
            }
            objitem->MapX = m_CurTileX;
            objitem->MapY = m_CurTileY;
            objitem->Dir = m_PlaceObjData.Dir;
            if(!AddItem(objitem))
            {
                delete objitem;
            }
        }
    }
    else if(m_isDeleteMode)
    {
        if(m_isNpc)
        {
        	long index = 0;
        	auto item = m_NpcList.GetItem(m_CurTileX, m_CurTileY, &index);
        	if(item)
			{
				MTC_Delete_Npc *cmd = new MTC_Delete_Npc(index);
				cmd->DeletedItem(item);
				m_commandProcessor.Store(cmd);
			}
            m_NpcList.DeleteItem(m_CurTileX, m_CurTileY);
            RefreshNpcList();
        }
        else if(m_isObj)
        {
        	long index = 0;
        	auto item = m_ObjList.GetItem(m_CurTileX, m_CurTileY, &index);
        	if(item)
			{
				MTC_Delete_Obj *cmd = new MTC_Delete_Obj(index);
				cmd->DeletedItem(item);
				m_commandProcessor.Store(cmd);
			}
            m_ObjList.DeleteItem(m_CurTileX, m_CurTileY);
            RefreshObjList();
        }
    }
    else if(m_isMoveMode)
    {
        if(m_isNpc)
		{
			long index = 0;
			m_MoveNpcItem = m_NpcList.GetItem(m_CurTileX, m_CurTileY, &index);
			mMoveCmdNpc.reset(new MTC_Move_Npc(index));
			mMoveCmdNpc->OldPos(m_CurTileX, m_CurTileY);
		}
        else if(m_isObj)
		{
			long index = 0;
			m_MoveObjItem = m_ObjList.GetItem(m_CurTileX, m_CurTileY, &index);
			mMoveCmdObj.reset(new MTC_Move_Obj(index));
			mMoveCmdObj->OldPos(m_CurTileX, m_CurTileY);
		}
    }
    else if(m_isEditAttribute)
    {
        if(m_isNpc)
        {
            long npcitemidx = 0;
            NpcItem *npcitem = m_NpcList.GetItem(m_CurTileX, m_CurTileY, &npcitemidx);
            ShowNpcItemEditor(npcitem, npcitemidx);
        }
        else if(m_isObj)
        {
            long objitemidx = 0;
            ObjItem *objitem = m_ObjList.GetItem(m_CurTileX, m_CurTileY, &objitemidx);
            ShowObjItemEditor(objitem, objitemidx);
        }
    }

    RedrawMapView();
}

bool MapTool::AddItem(NpcItem *item)
{
    if(!item) return false;
    if(m_NpcList.HasItem(item->MapX, item->MapY))
    {
        int ret = wxMessageBox(wxT("�滻��ǰNPC��"), wxT("��Ϣ"), wxYES_NO | wxCENTER | wxICON_INFORMATION);
        if(ret == wxYES)
        {
        	long index = 0;
        	NpcItem *deletedItem = m_NpcList.GetItem(item->MapX, item->MapY, &index);
        	MTC_Add_Npcs *cmd = new MTC_Add_Npcs(wxT("�滻Npc"));
        	cmd->DeleteAndAdd(index, deletedItem, item);
        	m_commandProcessor.Store(cmd);

            m_NpcList.DeleteItem(item->MapX, item->MapY);
            m_NpcList.AddItem(item);
        }
        else return false;
    }
    else
	{
		MTC_Add_Npcs *cmd = new MTC_Add_Npcs(wxT("���Npc"));
		cmd->Add(item);
		m_commandProcessor.Store(cmd);

		m_NpcList.AddItem(item);
    }
    RefreshNpcList();
    return true;
}

bool MapTool::AddItem(ObjItem *item)
{
    if(!item) return false;
    if(m_ObjList.HasItem(item->MapX, item->MapY))
    {
        int ret = wxMessageBox(wxT("�滻��ǰOBJ��"), wxT("��Ϣ"), wxYES_NO | wxCENTER | wxICON_INFORMATION);
        if(ret == wxYES)
        {
        	long index = 0;
        	ObjItem *deletedItem = m_ObjList.GetItem(item->MapX, item->MapY, &index);
        	MTC_Add_Objs *cmd = new MTC_Add_Objs(wxT("�滻Obj"));
        	cmd->DeleteAndAdd(index, deletedItem, item);
        	m_commandProcessor.Store(cmd);

            m_ObjList.DeleteItem(item->MapX, item->MapY);
            m_ObjList.AddItem(item);
        }
        else return false;
    }
    else
	{
		MTC_Add_Objs *cmd = new MTC_Add_Objs(wxT("���Obj"));
		cmd->Add(item);
		m_commandProcessor.Store(cmd);

		m_ObjList.AddItem(item);
	}
    RefreshObjList();
    return true;
}

void MapTool::OnUndo(wxCommandEvent& event)
{
	m_commandProcessor.Undo();
}

void MapTool::OnRedo(wxCommandEvent& event)
{
	m_commandProcessor.Redo();
}

void MapTool::ShowNpcItemEditor(long npcitemidx)
{
    NpcItem* npcitem = m_NpcList.GetItem(npcitemidx);
    ShowNpcItemEditor(npcitem, npcitemidx);
}

void MapTool::ShowNpcItemEditor(NpcItem* item, long index)
{
    if(item != NULL)
    {
        if(m_npcItemEdit) delete m_npcItemEdit;
        m_npcItemEdit = new NpcItemEditDialog(this,
                                              m_MapFileName.Mid(0, m_MapFileName.size() - 4),
                                              m_NpcAsfImgList,
                                              item);
        m_npcItemEdit->SetTitle(wxString::Format(wxT("NPC%03d"), index));
        m_npcItemEdit->InitFromNpcItem(item);
        NpcItemEditShowModle(m_npcItemEdit, item);
    }
}

void MapTool::NpcItemEditShowModle(NpcItemEditDialog* dialog, NpcItem *npcitem)
{
    int ret = dialog->ShowModal();
    if(ret == NpcItemEditDialog::OK)
	{
		MTC_Change_Npc *cmd = new MTC_Change_Npc(m_NpcList.GetIndex(npcitem));
		cmd->OldFrom(npcitem);
		dialog->AssignToNpcItem(npcitem);
		cmd->NewFrom(npcitem);
		m_commandProcessor.Store(cmd);
	}
    else if(ret == NpcItemEditDialog::FIXPOSEDIT)
        StartFixPosEdit(npcitem);
}

int MapTool::ShowYesNoAllInPosition(int tileX, int tileY, wxString message)
{
    ShowTile(tileX, tileY);
    RedrawNow();
    int posx, posy;
    MapPositionToScreenPositon(tileX, tileY, &posx, &posy);
    posx += 40;
    posy += 20;
    YesNoAllDialog dialog(this);
    dialog.Set(message.IsEmpty() ? wxT("ȷ���޸ģ�") : message, wxT("��Ϣ"), wxPoint(posx, posy));
    return dialog.ShowModal();
}

int MapTool::GetItemSelectionCount()
{
    if(m_isNpc)
    {
        return m_npcListCtrl->GetSelectedItemCount();
    }
    else if(m_isObj)
    {
        return m_objListCtrl->GetSelectedItemCount();
    }
    return 0;
}

void MapTool::ShowNpcItemEditor(const std::vector<long>& items)
{
    if(items.size() > 0)
    {
        NpcItem item;
        InitNpcItem(&item);
        NpcItemEditDialog dialog(this,
                                 m_MapFileName.Mid(0, m_MapFileName.size() - 4),
                                 m_NpcAsfImgList,
                                 &item);
        dialog.SetTitle(wxT("�����༭ģʽ��������ֵ����Ŀ������Ӧ��������ѡ��NPC"));
        dialog.EnableFixpos(false);
        dialog.InitFromNpcItem(&item);
        if(dialog.ShowModal() == NpcItemEditDialog::OK)
        {
        	MTC_Change_Npcs *cmd = new MTC_Change_Npcs();
            bool isAll = false;
            for(std::vector<long>::const_iterator it = items.begin();
                    it != items.end(); it++)
            {
                NpcItem *listItem = m_NpcList.GetItem((int)*it);
                if(listItem)
                {
                    if(!isAll)
                    {
                        int ret = ShowYesNoAllInPosition(listItem->MapX,
                                                         listItem->MapY);
                        if(ret == YesNoAllDialog::ALL)
                        {
                            isAll = true;
                        }
                        else if(ret == YesNoAllDialog::NO)
                        {
                            continue;
                        }
                        else if(ret == YesNoAllDialog::CANCLE)
                        {
                            break;
                        }
                    }
                    int index = m_NpcList.GetIndex(listItem);
                    cmd->OldFrom(index, listItem);
                    dialog.AssignToNpcItem(listItem, true);
                    cmd->NewFrom(index, listItem);
                }
            }
            if(cmd->size() > 0)
			{
				m_commandProcessor.Store(cmd);
			}
			else
			{
				delete cmd;
			}
        }
    }
}

void MapTool::ShowObjItemEditor(const std::vector<long>& items)
{
    if(items.size() == 0) return;
    ObjItem item;
    InitObjItem(&item);
    ObjItemEditDialog dialog(this,
                             m_MapFileName.Mid(0, m_MapFileName.size() - 4),
                             m_ObjAsfImgList,
                             &item);
    dialog.SetTitle(wxT("�����༭ģʽ��������ֵ����Ŀ������Ӧ��������ѡ��OBJ"));
    dialog.InitFromObjItem(&item);
    if(dialog.ShowModal() == wxID_OK)
    {
    	MTC_Change_Objs *cmd = new MTC_Change_Objs();
        bool isAll = false;
        for(std::vector<long>::const_iterator it = items.begin();
                it != items.end(); it++)
        {
            ObjItem *listItem = m_ObjList.GetItem((int)*it);
            if(listItem)
            {
                if(!isAll)
                {
                    int ret = ShowYesNoAllInPosition(listItem->MapX,
                                                     listItem->MapY);
                    if(ret == YesNoAllDialog::ALL)
                    {
                        isAll = true;
                    }
                    else if(ret == YesNoAllDialog::NO)
                    {
                        continue;
                    }
                    else if(ret == YesNoAllDialog::CANCLE)
                    {
                        break;
                    }
                }
                int index = m_ObjList.GetIndex(listItem);
                cmd->OldFrom(index, listItem);
                dialog.AssignToObjItem(listItem, true);
                cmd->NewFrom(index, listItem);
            }
        }
        if(cmd->size() > 0)
		{
			m_commandProcessor.Store(cmd);
		}
		else
		{
			delete cmd;
		}
    }
}

void MapTool::ShowObjItemEditor(long objitemidx)
{
    ObjItem* objitem = m_ObjList.GetItem(objitemidx);
    ShowObjItemEditor(objitem, objitemidx);
}

void MapTool::ShowObjItemEditor(ObjItem* item, long index)
{
    if(item != NULL)
    {
        ObjItemEditDialog itemEdit(this,
                                   m_MapFileName.Mid(0, m_MapFileName.size() - 4),
                                   m_ObjAsfImgList,
                                   item);
        itemEdit.SetTitle(wxString::Format(wxT("OBJ%03d"), index));
        itemEdit.InitFromObjItem(item);
        if(itemEdit.ShowModal() == wxID_OK)
		{
			MTC_Change_Obj *cmd = new MTC_Change_Obj(m_ObjList.GetIndex(item));
			cmd->OldFrom(item);
			itemEdit.AssignToObjItem(item);
			cmd->NewFrom(item);
			m_commandProcessor.Store(cmd);
		}
    }
}

void MapTool::OnMapViewMouseLeftUp( wxMouseEvent& event )
{
	int curTileX = m_CurTileX;
	int curTileY = m_CurTileY;
	if(m_isNpc && m_npcsToPasted.getCounts() > 0)
	{
		//multy paste
		NpcList templist;
		m_npcsToPasted.HandoverOwnship(templist); //clear to pasted list to indicate multy pasting mode end
		bool isAll = false;
		bool isAllNo = false;

		MTC_Add_Npcs *cmd = new MTC_Add_Npcs(wxT("����ճ��Npc"));

		for(auto& npcitem : templist)
		{
			npcitem->MapX = npcitem->MapX - m_tilePositionOffsetNpc.x + curTileX;
			npcitem->MapY = npcitem->MapY - m_tilePositionOffsetNpc.y + curTileY;

			bool historyAdded = false;
			if(m_NpcList.HasItem(npcitem->MapX, npcitem->MapY))
			{
				if(!isAll)
				{
					if(isAllNo)
					{
						delete npcitem;
						continue;
					}

                    int ret = ShowYesNoAllInPosition(npcitem->MapX,
                    npcitem->MapY, wxT("�滻��ǰNPC��"));
                    if(ret == YesNoAllDialog::ALL)
                    {
                        isAll = true;
                    }
                    else if(ret == YesNoAllDialog::NO)
                    {
                    	delete npcitem;
                        continue;
                    }
                    else if(ret == YesNoAllDialog::CANCLE)
                    {
                    	delete npcitem;
                    	isAllNo = true;
                        continue;
                    }
				}

				long index = 0;
				NpcItem *deletedItem = m_NpcList.GetItem(npcitem->MapX, npcitem->MapY, &index);
				cmd->DeleteAndAdd(index, deletedItem, npcitem);
				historyAdded = true;

				m_NpcList.DeleteItem(npcitem->MapX, npcitem->MapY);
			}

			if(!historyAdded)
			{
				cmd->Add(npcitem);
			}

			m_NpcList.AddItem(npcitem);
		}

		m_commandProcessor.Store(cmd);

		templist.ClearWithoutFreeData();
		RefreshNpcList();

	}
	else if(m_isObj && m_objsToPasted.getCounts() > 0)
	{
		//multy paste
		ObjList templist;
		m_objsToPasted.HandoverOwnship(templist); //clear to pasted list to indicate multy pasting mode end
		bool isAll = false;
		bool isAllNo = false;

		MTC_Add_Objs *cmd = new MTC_Add_Objs(wxT("����ճ��Obj"));

		for(auto& objitem : templist)
		{
			objitem->MapX = objitem->MapX - m_tilePositionOffsetObj.x + curTileX;
			objitem->MapY = objitem->MapY - m_tilePositionOffsetObj.y + curTileY;

			bool historyAdded = false;
			if(m_ObjList.HasItem(objitem->MapX, objitem->MapY))
			{
				if(!isAll)
				{
					if(isAllNo)
					{
						delete objitem;
						continue;
					}

                    int ret = ShowYesNoAllInPosition(objitem->MapX,
                    objitem->MapY, wxT("�滻��ǰOBJ��"));
                    if(ret == YesNoAllDialog::ALL)
                    {
                        isAll = true;
                    }
                    else if(ret == YesNoAllDialog::NO)
                    {
                    	delete objitem;
                        continue;
                    }
                    else if(ret == YesNoAllDialog::CANCLE)
                    {
                    	delete objitem;
                    	isAllNo = true;
                        continue;
                    }
				}

				long index = 0;
				ObjItem *deletedItem = m_ObjList.GetItem(objitem->MapX, objitem->MapY, &index);
				cmd->DeleteAndAdd(index, deletedItem, objitem);
				historyAdded = true;

				m_ObjList.DeleteItem(objitem->MapX, objitem->MapY);
			}

			if(!historyAdded)
			{
				cmd->Add(objitem);
			}

			m_ObjList.AddItem(objitem);
		}

		m_commandProcessor.Store(cmd);

		templist.ClearWithoutFreeData();
		RefreshObjList();
	}
    else if(IsInSelectingItem())
    {
        if(m_batchSelectionBeginTile != INVALID_TILE_POSITION && m_batchSelectionEndTile != INVALID_TILE_POSITION)
		{
			DoBatchRectangleSelection();
		}
    }
    else
	{
		long posX, posY;
		event.GetPosition(&posX, &posY);
		if(m_isEditFixPos)
		{
			int tileX, tileY;
			if(ToTilePositionFromWindowPositon(posX, posY, &tileX, &tileY))
			{
				if(tileX != 0 && tileY != 0)
				{
					int pixelX, pixelY;
					if(map.GetTileCenterPixelPosition(tileX, tileY, &pixelX, &pixelY))
					{
						wxPoint pos = wxPoint(pixelX, pixelY);
						m_fixPosPoints.push_back(pos);
						if(m_fixPosPoints.size() == 8)
						{
							EndFixPosEdit();
						}
					}
				}
			}
		}
		else if(m_isMoveMode)
		{
			if(m_isNpc)
			{
				if(m_MoveNpcItem)
				{
					mMoveCmdNpc->NewPos(m_MoveNpcItem->MapX, m_MoveNpcItem->MapY);
					if(!mMoveCmdNpc->IsNewOldEqual())
					{
						m_commandProcessor.Store(mMoveCmdNpc.release());
					}
					UpdateListItem(m_MoveNpcItem);
					m_MoveNpcItem = NULL;
				}
			}
			else if(m_isObj)
			{
				if(m_MoveObjItem)
				{
					mMoveCmdObj->NewPos(m_MoveObjItem->MapX, m_MoveObjItem->MapY);
					if(!mMoveCmdObj->IsNewOldEqual())
					{
						m_commandProcessor.Store(mMoveCmdObj.release());
					}
					UpdateListItem(m_MoveObjItem);
					m_MoveObjItem = NULL;
				}
			}
		}
	}
}

void MapTool::SetMapViewPopupMenuState(bool hasItem, bool canPaste, bool isMulty)
{
    m_menuMapView->Enable(MYID_MAPVIEW_COPY, hasItem );
    m_menuMapView->Enable(MYID_MAPVIEW_CUT, hasItem);
    m_menuMapView->Enable(MYID_MAPVIEW_PASTE, canPaste);
    m_menuMapView->Enable(MYID_MAPVIEW_DETIAL, hasItem && !isMulty);
    m_menuMapView->Enable(MYID_MAPVIEW_DELETE, hasItem);
    int count = GetItemSelectionCount();
    m_menuMapView->Enable(MYID_MAPVIEW_BATEDIT, count > 0);
    m_menuMapView->Enable(MYID_MAPVIEW_CLEAR_SELECTION, count >0);
}

void MapTool::PopupMapViewMenu()
{
    m_popupMenuShowed = true;
    m_isBatch = IsInSelectingItem();
    m_menuMapView->SetLabel(MYID_MAPVIEW_COPY, m_isBatch ? wxT("��������") : wxT("����"));
    m_menuMapView->SetLabel(MYID_MAPVIEW_CUT, m_isBatch ? wxT("��������") : wxT("����"));
    m_menuMapView->SetLabel(MYID_MAPVIEW_DELETE, m_isBatch ? wxT("����ɾ��") : wxT("ɾ��"));
    if(m_isNpc)
	{
		m_menuMapView->SetLabel(MYID_MAPVIEW_BATEDIT, wxT("NPC_�����༭..."));
		m_menuMapView->SetLabel(MYID_MAPVIEW_CLEAR_SELECTION, wxT("NPC_����ѡ��"));
	}
	else if(m_isObj)
	{
		m_menuMapView->SetLabel(MYID_MAPVIEW_BATEDIT, wxT("OBJ_�����༭..."));
		m_menuMapView->SetLabel(MYID_MAPVIEW_CLEAR_SELECTION, wxT("OBJ_����ѡ��"));
	}
    PopupMenu(m_menuMapView);
}

void MapTool::OnMapViewMouseRightUp(wxMouseEvent& event)
{
	//Disable timer in this scop
	TimerDisabler disableTimer(this);

	if(m_isNpc && m_npcsToPasted.getCounts() > 0)
	{
		m_npcsToPasted.Clear();
	}
	else if(m_isObj && m_objsToPasted.getCounts() > 0)
	{
		//Cnacle batch paste
		m_objsToPasted.Clear();
	}
    else if(m_isEditFixPos)
    {
        EndFixPosEdit(true);
    }
    else if(!m_MapFileName.IsEmpty())// Map is loaded
    {
    	bool hasItem = false;
    	bool isMulty = IsInSelectingItem();
    	bool selectedCount = GetItemSelectionCount();
    	m_selectedNpcItemByRightUp = nullptr;
    	m_selectedObjItemByRightUp = nullptr;
        if(m_isNpc)
        {
        	if(isMulty)
			{
				//Batch copy/cut
				hasItem = selectedCount > 0;
			}
			else
			{
				NpcItem *item = m_NpcList.GetItem(m_CurTileX, m_CurTileY);
				if(item)
				{
					m_selectedNpcItemByRightUp = item;
				}
				hasItem = item != nullptr;
			}

            SetMapViewPopupMenuState(hasItem, m_clipBoard.HasNpc(), isMulty);
            PopupMapViewMenu();
        }
        else if(m_isObj)
        {
        	if(isMulty)
			{
				//Batch copy/cut
				hasItem = selectedCount > 0;
			}
			else
			{
				ObjItem *item = m_ObjList.GetItem(m_CurTileX, m_CurTileY);
				if(item)
				{
					m_selectedObjItemByRightUp = item;
				}
				hasItem = item != nullptr;
			}

            SetMapViewPopupMenuState(hasItem, m_clipBoard.HasObj(), isMulty);
            PopupMapViewMenu();
        }
    }
}

bool MapTool::ToTilePositionFromWindowPositon(int windowX, int windowY, int* tileX, int* tileY)
{
    return map.GetTilePosition(windowX + m_ViewBeginx, windowY + m_ViewBeginy, tileX, tileY);
}

void MapTool::OnMouseMove( wxMouseEvent& event )
{
    long posx, posy, lasttilex = m_CurTileX, lasttiley = m_CurTileY;
    wxString msg;
    event.GetPosition(&posx, &posy);

    if(ToTilePositionFromWindowPositon(posx, posy, &m_CurTileX, &m_CurTileY) &&
            (lasttilex != m_CurTileX || lasttiley != m_CurTileY) )
    {
        msg = wxString::Format(wxT("[%ld,%ld]"), m_CurTileX, m_CurTileY);

        if(m_isEditFixPos)
        {
            //Do nothing
        }
        else if(m_isMoveMode && event.Dragging())
        {
            if(m_CurTileX >=0 && m_CurTileX < map.getCol())
            {
                if(m_isNpc && m_MoveNpcItem != NULL)
                    m_MoveNpcItem->MapX = m_CurTileX;
                else if(m_isObj && m_MoveObjItem != NULL)
                    m_MoveObjItem->MapX = m_CurTileX;
            }
            if(m_CurTileY >= 0 && m_CurTileY < map.getRow())
            {
                if(m_isNpc && m_MoveNpcItem != NULL)
                    m_MoveNpcItem->MapY = m_CurTileY;
                else if(m_isObj && m_MoveObjItem != NULL)
                    m_MoveObjItem->MapY = m_CurTileY;
            }
            CorrectFixedPos(m_MoveNpcItem);
        }

        if(IsInSelectingItem())
		{
			if(!wxGetMouseState().LeftIsDown() &&
				m_batchSelectionBeginTile != INVALID_TILE_POSITION &&
				m_batchSelectionEndTile != INVALID_TILE_POSITION)
			{
				//Mouse left button released at map view outer.
				DoBatchRectangleSelection();
			}
			else if(m_batchSelectionBeginTile != INVALID_TILE_POSITION)
			{
				m_batchSelectionEndTile = wxPoint(m_CurTileX, m_CurTileY);
			}
		}

        RedrawMapView();

        m_StatusBar->SetStatusText(msg, 0);
    }
}

void MapTool::OnDrawMapControl( wxPaintEvent& event )
{
    if(m_MapFileName.IsEmpty())
    {
        event.Skip();
        return;
    }

    int ctlwidth, ctlheight, mapviewwidth, mapviewheight, mapwidth, mapheight;

    m_MapControl->GetClientSize(&ctlwidth, &ctlheight);
    m_MapView->GetClientSize(&mapviewwidth, &mapviewheight);
    mapwidth = m_MapBitmap.GetWidth();
    mapheight = m_MapBitmap.GetHeight();

    if(mapwidth == 0 || mapheight == 0) return;

    wxMemoryDC memdc;
    memdc.SelectObject(m_MapBitmap);
    wxPaintDC dc(m_MapControl);

    dc.StretchBlit(0,
                   0,
                   ctlwidth,
                   ctlheight,
                   &memdc,
                   0,
                   0,
                   mapwidth,
                   mapheight
                  );

    int recwidth, recheight, recbegx, recbegy;
    recwidth = ctlwidth * mapviewwidth / mapwidth;
    recheight = ctlheight * mapviewheight / mapheight;
    recbegx = ctlwidth * m_ViewBeginx / mapwidth;
    recbegy = ctlheight * m_ViewBeginy / mapheight;
    wxPoint points[4];
    points[0] = wxPoint(0, 0);
    points[1] = wxPoint(recwidth, 0);
    points[2] = wxPoint(recwidth, recheight);
    points[3] = wxPoint(0, recheight);
    dc.SetPen(*(wxThePenList->FindOrCreatePen(*wxRED)));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawPolygon(4,
                   points,
                   recbegx,
                   recbegy);

}

void MapTool::OnMapCtrlMouseMotion( wxMouseEvent& event )
{
    if(!event.Dragging() || m_MapFileName.IsEmpty()) return;

    int ctlwidth, ctlheight,
        mapviewwidth, mapviewheight,
        mapwidth, mapheight,
        mousex, mousey;

    m_MapControl->GetClientSize(&ctlwidth, &ctlheight);
    m_MapView->GetClientSize(&mapviewwidth, &mapviewheight);
    mapwidth = m_MapBitmap.GetWidth();
    mapheight = m_MapBitmap.GetHeight();
    event.GetPosition(&mousex, &mousey);

    int recwidth, recheight, recbegx, recbegy;
    recwidth = ctlwidth * mapviewwidth / mapwidth;
    recheight = ctlheight * mapviewheight / mapheight;
    recbegx = mousex - recwidth / 2;
    recbegy = mousey - recheight /2;

    if(recbegx + recwidth > ctlwidth) recbegx = ctlwidth - recwidth;
    if(recbegy + recheight > ctlheight) recbegy = ctlheight - recheight;
    if(recbegx < 0) recbegx = 0;
    if(recbegy < 0) recbegy = 0;

    m_ViewBeginx = recbegx * mapwidth / ctlwidth;
    m_ViewBeginy = recbegy * mapheight / ctlheight;

    RedrawMapView();

}

void MapTool::OnLoadCharater( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("ѡ��һ������ini�ļ�"),
                         exepath + wxT("ini\\npc\\"),
                         wxT(""),
                         wxT("ini�ļ�(*.ini)|*.ini"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if(filedlg.ShowModal() != wxID_OK) return;
    m_NpcIniFilePath = filedlg.GetPath();
    ReadIni(exepath, m_NpcIniFilePath, &m_PlaceNpcData);
    m_PlaceNpcData.FixedPos = wxEmptyString; //FixedPos is useless
}
void MapTool::OnLoadObject( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("ѡ��һ����Ʒini�ļ�"),
                         exepath + wxT("ini\\obj\\"),
                         wxT(""),
                         wxT("ini�ļ�(*.ini)|*.ini"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if(filedlg.ShowModal() != wxID_OK) return;
    m_ObjIniFilePath = filedlg.GetPath();
    ReadIni(exepath, m_ObjIniFilePath, NULL, &m_PlaceObjData);
}
void MapTool::OnCharacterDirection( wxCommandEvent& event )
{
    if(m_isNpc)
    {
        m_PlaceNpcData.Dir++;
        if(m_PlaceNpcData.NpcStand->GetFramesCounts() != 0 && m_PlaceNpcData.NpcStand->GetDirection() != 0)
            m_PlaceNpcData.Dir %= m_PlaceNpcData.NpcStand->GetDirection();
        else
            m_PlaceNpcData.Dir %= 8;
    }
    else if(m_isObj)
    {
        m_PlaceObjData.Dir++;
        if(m_PlaceObjData.ObjCommon->GetFramesCounts() != 0 && m_PlaceObjData.ObjCommon->GetDirection() != 0)
            m_PlaceObjData.Dir %= m_PlaceObjData.ObjCommon->GetDirection();
        else
            m_PlaceObjData.Dir %= 8;
    }

    RedrawMapView();
}
void MapTool::OnImportNpcFile( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("ѡ��һ��NPC�ļ�"),
                         m_NpcObjPath,
                         m_LastNpcListFileName,
                         wxT("NPC�ļ�(*.npc)|*.npc"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST
                        );

    if(filedlg.ShowModal() == wxID_OK)
    {
        m_NpcObjPath = filedlg.GetDirectory() + wxT("\\");
        m_LastNpcListFileName = wxFileName(filedlg.GetFilename()).GetFullName();

        if(NpcListImport(exepath, filedlg.GetPath(), &m_NpcList, m_NpcAsfImgList, &m_commandProcessor))
        {
            wxMessageBox(wxT("���"), wxT("��Ϣ"));
            RedrawMapView();
            RefreshNpcList();
        }
        else
            wxMessageBox(wxT("ʧ��"), wxT("����"), wxOK | wxCENTER | wxICON_ERROR);
    }
}
void MapTool::OnOutputNpcFile( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("����ΪNPC�ļ�"),
                         m_NpcObjPath,
                         m_LastNpcListFileName,
                         wxT("NPC�ļ�(*.npc)|*.npc"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                        );

    if(filedlg.ShowModal() == wxID_OK)
    {
        m_NpcObjPath = filedlg.GetDirectory() + wxT("\\");
        m_LastNpcListFileName = wxFileName(filedlg.GetFilename()).GetFullName();

        if(NpcListSave(filedlg.GetPath(), m_MapFileName, &m_NpcList))
            wxMessageBox(wxT("���"), wxT("��Ϣ"));
        else
            wxMessageBox(wxT("ʧ��"), wxT("����"), wxOK | wxCENTER | wxICON_ERROR);
    }
}
void MapTool::OnRepositionUnseenNpc(wxCommandEvent& event)
{
	if(wxMessageBox(wxT("�ò�����ѵ�ͼ��ģ��޷�������NPC���ƶ�����ͼ�����Ͻǣ��Ƿ�ִ�У�"),
					wxT("��Ϣ"),
					wxOK | wxCANCEL |wxCENTER | wxICON_QUESTION)
		== wxOK)
	{
		MTC_Move_Npcs *cmd = new MTC_Move_Npcs(wxT("�޸�����λ��"));
		if(m_NpcList.getCounts() > 0 && map.getRow() > 0 && map.getCol() > 0)
		{
			long width = map.getCol();
			long height = map.getRow();
			NpcItem *item, *lastItem;
			long x = 0, y = 4;
			long maxX = width > 15 ? 15 : width;
			bool moved = false;
			size_t index = 0;
			for(NpcList::iterator it = m_NpcList.begin(); it != m_NpcList.end(); it++, index++)
			{
				item = *it;
				if(item == nullptr) continue;
				if(item->MapX < 0 || item->MapY < 0 || item->MapX > width || item->MapY > height)
				{
					cmd->Move(index, item->MapX, item->MapY, x, y);

					moved = true;
					lastItem = item;
					item->MapX = x;
					item->MapY = y;
					CorrectFixedPos(item);
					UpdateListItem(index, NPCLIST);

					x++;
					if(x > maxX)
					{
						x = 0;
						y++;
					}
				}
			}

			if(moved)
			{
				ShowTile(lastItem->MapX, lastItem->MapY);
			}
		}
		m_commandProcessor.Store(cmd);
	}
}
void MapTool::OnImportObjFile( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("ѡ��һ��OBJ�ļ�"),
                         m_NpcObjPath,
                         m_LastObjListFileName,
                         wxT("OBJ�ļ�(*.obj)|*.obj"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST
                        );

    if(filedlg.ShowModal() == wxID_OK)
    {
        m_NpcObjPath = filedlg.GetDirectory() + wxT("\\");
        m_LastObjListFileName = wxFileName(filedlg.GetFilename()).GetFullName();

        if(ObjListImport(exepath, filedlg.GetPath(), &m_ObjList, m_ObjAsfImgList, &m_commandProcessor))
        {
            wxMessageBox(wxT("���"), wxT("��Ϣ"));
            RedrawMapView();
            RefreshObjList();
        }
        else
            wxMessageBox(wxT("ʧ��"), wxT("����"), wxOK | wxCENTER | wxICON_ERROR);
    }
}
void MapTool::OnOutputObjFile( wxCommandEvent& event )
{
    wxFileDialog filedlg(this,
                         wxT("����ΪOBJ�ļ�"),
                         m_NpcObjPath,
                         m_LastObjListFileName,
                         wxT("OBJ�ļ�(*.obj)|*.obj"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                        );

    if(filedlg.ShowModal() == wxID_OK)
    {
        m_NpcObjPath = filedlg.GetDirectory() + wxT("\\");
        m_LastObjListFileName = wxFileName(filedlg.GetFilename()).GetFullName();

        if(ObjListSave(filedlg.GetPath(), m_MapFileName, &m_ObjList))
            wxMessageBox(wxT("���"), wxT("��Ϣ"));
        else
            wxMessageBox(wxT("ʧ��"), wxT("����"), wxOK | wxCENTER | wxICON_ERROR);
    }
}
void MapTool::OnRepositionUnseenObj(wxCommandEvent& event)
{
	if(wxMessageBox(wxT("�ò�����ѵ�ͼ��ģ��޷�������OBJ���ƶ�����ͼ�����Ͻǣ��Ƿ�ִ�У�"),
					wxT("��Ϣ"),
					wxOK | wxCANCEL |wxCENTER | wxICON_QUESTION)
		== wxOK)
	{
		MTC_Move_Objs *cmd = new MTC_Move_Objs(wxT("�޸���Ʒλ��"));
		if(m_ObjList.getCounts() > 0 && map.getRow() > 0 && map.getCol() > 0)
		{
			long width = map.getCol();
			long height = map.getRow();
			ObjItem *item, *lastItem;
			long x = width, y = 4;
			long minX = width < 15 ? 0 : (width - 15);
			bool moved = false;
			size_t index = 0;
			for(ObjList::iterator it = m_ObjList.begin(); it != m_ObjList.end(); it++,index++)
			{
				item = *it;
				if(item == nullptr) continue;
				if(item->MapX < 0 || item->MapY < 0 || item->MapX > width || item->MapY > height)
				{
					cmd->Move(index, item->MapX, item->MapY, x, y);

					moved = true;
					lastItem = item;
					item->MapX = x;
					item->MapY = y;
					UpdateListItem(index, OBJLIST);

					x--;
					if(x <= minX)
					{
						x = width;
						y++;
					}
				}
			}

			if(moved)
			{
				ShowTile(lastItem->MapX, lastItem->MapY);
			}
		}
		m_commandProcessor.Store(cmd);
	}
}
void MapTool::OnPlaceMode( wxCommandEvent& event )
{
    m_ToolBarEdit->ToggleTool(ID_TOOLPLACE, true);
    m_ToolBarEdit->ToggleTool(ID_TOOLDELETE, false);
    m_ToolBarEdit->ToggleTool(ID_EDITATTRIBUTE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLMOVE, false);

    m_isPlaceMode = true;
    m_isDeleteMode = false;
    m_isEditAttribute = false;
    m_isMoveMode = false;

    RedrawMapView();
}
void MapTool::OnDeleteMode( wxCommandEvent& event )
{
    m_ToolBarEdit->ToggleTool(ID_TOOLPLACE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLDELETE, true);
    m_ToolBarEdit->ToggleTool(ID_EDITATTRIBUTE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLMOVE, false);

    m_isPlaceMode = false;
    m_isDeleteMode = true;
    m_isEditAttribute = false;
    m_isMoveMode = false;

    RedrawMapView();
}
void MapTool::OnEditAttributeMode( wxCommandEvent& event )
{
    m_ToolBarEdit->ToggleTool(ID_TOOLPLACE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLDELETE, false);
    m_ToolBarEdit->ToggleTool(ID_EDITATTRIBUTE, true);
    m_ToolBarEdit->ToggleTool(ID_TOOLMOVE, false);

    m_isPlaceMode = false;
    m_isDeleteMode = false;
    m_isEditAttribute = true;
    m_isMoveMode = false;

    RedrawMapView();
}
void MapTool::OnMoveMode( wxCommandEvent& event )
{
    m_ToolBarEdit->ToggleTool(ID_TOOLPLACE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLDELETE, false);
    m_ToolBarEdit->ToggleTool(ID_EDITATTRIBUTE, false);
    m_ToolBarEdit->ToggleTool(ID_TOOLMOVE, true);

    m_isPlaceMode = false;
    m_isDeleteMode = false;
    m_isEditAttribute = false;
    m_isMoveMode = true;

    RedrawMapView();
}

void MapTool::CheckMapViewBeginPosition()
{
    int viewWidth, viewHeight, bmpWidth, bmpHeight;
    m_MapView->GetClientSize(&viewWidth, &viewHeight);
    bmpWidth = m_MapBitmap.GetWidth();
    bmpHeight = m_MapBitmap.GetHeight();
    if(m_ViewBeginx + viewWidth > bmpWidth) m_ViewBeginx = bmpWidth - viewWidth;
    if(m_ViewBeginy + viewHeight > bmpHeight) m_ViewBeginy = bmpHeight - viewHeight;
    if(m_ViewBeginx < 0) m_ViewBeginx = 0;
    if(m_ViewBeginy < 0) m_ViewBeginy = 0;
}
void MapTool::OnListCtrlLeftDClick(wxMouseEvent& event)
{
    int id = event.GetId();
    wxPoint point = event.GetPosition();
    int flag = wxLIST_HITTEST_ONITEMLABEL;
    if(id == MYID_NPCLISTCTRL)
    {
        if(m_npcListCtrl->HitTest(point, flag) == wxNOT_FOUND)
            return;
        m_curList = NPCLIST;
        m_curListIndex = m_npcListCtrl->GetFocusedItem();
    }
    else if(id == MYID_OBJLISTCTRL)
    {
        if(m_objListCtrl->HitTest(point, flag) == wxNOT_FOUND)
            return;
        m_curList = OBJLIST;
        m_curListIndex = m_objListCtrl->GetFocusedItem();
    }
    else
    {
        m_curList = -1;
        m_curListIndex = -1;
    }

    ShowAttributeListItem();
}
void MapTool::OnListCtrlRightDown(wxListEvent& event)
{
    if(m_isEditFixPos) return;

	//Disable timer in this scop
	TimerDisabler disableTimer(this);

    int id = event.GetId();
    if(id == MYID_NPCLISTCTRL)
    {
        m_curList = NPCLIST;
    }
    else if(id == MYID_OBJLISTCTRL)
    {
        m_curList = OBJLIST;
    }
    else
        m_curList = 0;
    m_curListIndex = event.GetIndex();

    wxMenu menu;
    menu.Append(ID_ATTRIBUTELISTITEM, wxT("��ϸ..."));
    menu.Append(ID_DELETELISTITEM, wxT("ɾ��"));
    menu.AppendSeparator();
    menu.Append(ID_BATEDITLISTITEM, wxT("�����༭..."));
    PopupMenu(&menu);
}
void MapTool::OnListItemFocused(wxListEvent& event)
{
    int id = event.GetId();
    int itemidx, mapX, mapY;
    bool found = false;
    if(id == MYID_NPCLISTCTRL)
    {
        m_curList = NPCLIST;

        NpcItem *item;
        itemidx = (int)m_npcListCtrl->GetFocusedItem();
        if(itemidx != -1)
        {
            item = m_NpcList.GetItem(itemidx);
            if(item)
            {
                found = true;
                mapX = item->MapX;
                mapY = item->MapY;
            }
        }
    }
    else if(id == MYID_OBJLISTCTRL)
    {
        m_curList = OBJLIST;

        ObjItem *item;
        itemidx = m_objListCtrl->GetFocusedItem();
        if(itemidx != -1)
        {
            item = m_ObjList.GetItem(itemidx);
            if(item)
            {
                found = true;
                mapX = item->MapX;
                mapY = item->MapY;
            }
        }
    }
    if(found)
    {
        m_curListIndex = itemidx;

        ShowTile(mapX, mapY);
    }
}

void MapTool::RefreshNpcList()
{
    NpcItem **items = m_NpcList.GetAllItem();
    m_npcListCtrl->DeleteAllItems();
    for(int i = 0; items[i] != NULL; i++)
    {
        m_npcListCtrl->InsertItem(i, "");
        m_npcListCtrl->SetItem(i, 0, wxString::Format("%d", i));
        m_npcListCtrl->SetItem(i, 1, wxString::Format("%ld", items[i]->MapX));
        m_npcListCtrl->SetItem(i, 2, wxString::Format("%ld", items[i]->MapY));
    }
    delete[] items;
}
void MapTool::RefreshObjList()
{
    ObjItem **items = m_ObjList.GetAllItem();
    m_objListCtrl->DeleteAllItems();
    for(int i = 0; items[i] != NULL; i++)
    {
        m_objListCtrl->InsertItem(i, "");
        m_objListCtrl->SetItem(i, 0, wxString::Format("%d", i));
        m_objListCtrl->SetItem(i, 1, wxString::Format("%ld", items[i]->MapX));
        m_objListCtrl->SetItem(i, 2, wxString::Format("%ld", items[i]->MapY));
    }
    delete[] items;
}
void MapTool::ShowTile(int tileX, int tileY)
{
    int pixelX, pixelY;
    if(map.GetPixelPosition((int)tileX, (int)tileY, &pixelX, &pixelY))
    {
        int viewWidth, viewHeight;
        m_MapView->GetClientSize(&viewWidth, &viewHeight);
        m_ViewBeginx = pixelX - viewWidth / 2;
        m_ViewBeginy = pixelY - viewHeight / 2;
        CheckMapViewBeginPosition();
        m_CurTileX = tileX;
        m_CurTileY = tileY;

        m_placeModeNotDraw = true;
        RedrawMapView();
    }
}
void MapTool::DeleteListItem(wxCommandEvent& event)
{
    if(!PromptDelection())
    {
        return;
    }
    if(m_curList == NPCLIST)
    {
    	NpcItem *item = m_NpcList.GetItem(m_curListIndex);
    	if(item)
		{
			MTC_Delete_Npc *cmd = new MTC_Delete_Npc(m_curListIndex);
			cmd->DeletedItem(item);
			m_commandProcessor.Store(cmd);
		}

        DeleteNpcItem(m_curListIndex);
    }
    else if(m_curList == OBJLIST)
    {
    	ObjItem *item = m_ObjList.GetItem(m_curListIndex);
    	if(item)
		{
			MTC_Delete_Obj *cmd = new MTC_Delete_Obj(m_curListIndex);
			cmd->DeletedItem(item);
			m_commandProcessor.Store(cmd);
		}

        DeleteObjItem(m_curListIndex);
    }
    m_placeModeNotDraw = true;
}

void MapTool::DeleteNpcItem(int index)
{
    m_NpcList.DeleteItem(index);
    RefreshNpcList();
    RedrawMapView();
}

void MapTool::DeleteObjItem(int index)
{
    m_ObjList.DeleteItem(index);
    RefreshObjList();
    RedrawMapView();
}

void MapTool::AttributeListItem(wxCommandEvent& event)
{
    ShowAttributeListItem();
}

void MapTool::BatEditListItem(wxCommandEvent& event)
{
    if(m_curList == NPCLIST)
    {
        ShowNpcItemEditor(GetAllSelectedItems(m_npcListCtrl));
    }
    else if(m_curList == OBJLIST)
    {
        ShowObjItemEditor(GetAllSelectedItems(m_objListCtrl));
    }
}

std::vector<long> GetAllSelectedItems(const wxListView *listCtrl)
{
    std::vector<long> v;
    if(!listCtrl) return v;
    long i = listCtrl->GetFirstSelected();

    while(true)
    {
        if(i != -1)
        {
            v.push_back(i);
        }
        else
        {
            break;
        }
        i = listCtrl->GetNextSelected(i);
    }
    return v;
}


void MapTool::ShowAttributeListItem()
{
    //Can't show when edit npc FixPos.
    if(m_isEditFixPos) return;

    if(m_curList == NPCLIST)
    {
        ShowNpcItemEditor(m_curListIndex);
    }
    else if(m_curList == OBJLIST)
    {
        ShowObjItemEditor(m_curListIndex);
    }
    RedrawMapView();
}

void MapTool::ShowBatEditDialog()
{
    if(m_isNpc)
    {
        ShowNpcItemEditor(GetAllSelectedItems(m_npcListCtrl));
    }
    else if(m_isObj)
    {
        ShowObjItemEditor(GetAllSelectedItems(m_objListCtrl));
    }
}

void MapTool::UpdateListItem(int index, int ls)
{
    if(ls == NPCLIST)
    {
        NpcItem *item = m_NpcList.GetItem(index);
        if(item)
        {
            m_npcListCtrl->SetItem(index, 1, wxString::Format("%ld", item->MapX));
            m_npcListCtrl->SetItem(index, 2, wxString::Format("%ld", item->MapY));
        }
    }
    else if(ls == OBJLIST)
    {
        ObjItem *item = m_ObjList.GetItem(index);
        if(item)
        {
            m_objListCtrl->SetItem(index, 1, wxString::Format("%ld", item->MapX));
            m_objListCtrl->SetItem(index, 2, wxString::Format("%ld", item->MapY));
        }
    }
}
void MapTool::UpdateListItem(NpcItem* item)
{
    if(item)
    {
        long idx;
        if(m_NpcList.GetItem(item->MapX, item->MapY, &idx))
        {
            UpdateListItem((int)idx, NPCLIST);
        }
    }
}

void MapTool::UpdateListItem(ObjItem* item)
{
    if(item)
    {
        long idx;
        if(m_ObjList.GetItem(item->MapX, item->MapY, &idx))
        {
            UpdateListItem((int)idx, OBJLIST);
        }
    }
}

void MapTool::ToggleNpcSelection(int tileX, int tileY)
{
    long index;
    if(m_NpcList.GetItem(tileX, tileY, &index))
    {
        m_npcListCtrl->Select(index, !m_npcListCtrl->IsSelected(index));
        ShowNpcSelectionCountMessage();
    }
}

void MapTool::ToggleObjSelection(int tileX, int tileY)
{
    long index;
    if(m_ObjList.GetItem(tileX, tileY, &index))
    {
        m_objListCtrl->Select(index, !m_objListCtrl->IsSelected(index));
		ShowObjSelectionCountMessage();
    }
}

void MapTool::ShowNpcSelectionCountMessage()
{
	m_StatusBar->SetStatusText(
            wxString::Format(wxT("ѡ���� %d ��"), m_npcListCtrl->GetSelectedItemCount())
            , 0);
}

void MapTool::ShowObjSelectionCountMessage()
{
	m_StatusBar->SetStatusText(
            wxString::Format(wxT("ѡ���� %d ��"), m_objListCtrl->GetSelectedItemCount())
            , 0);
}

void MapTool::DoBatchRectangleSelection()
{
	int sX, sY, eX, eY;
	if( m_batchSelectionBeginTile != m_batchSelectionEndTile &&
		map.GetPixelPosition(m_batchSelectionBeginTile.x, m_batchSelectionBeginTile.y, &sX, &sY) &&
		map.GetPixelPosition(m_batchSelectionEndTile.x, m_batchSelectionEndTile.y, &eX, &eY))
	{
		int x = std::min(sX, eX);
		int y = std::min(sY, eY);
		int w = std::abs(sX - eX);
		int h = std::abs(sY - eY);
		int itemX, itemY;
		wxRect rect(x,y,w,h);
		if(m_isNpc)
		{
			bool isAdd = wxGetKeyState(SELECTION_ADD_KEY);
			bool isSub = wxGetKeyState(SELECTION_SUB_KEY);
			if(!isAdd && !isSub)
			{
				//clear all selected item
				DeselectAllItem(m_npcListCtrl);
			}

			long index = 0;
			for(auto it = m_NpcList.begin(); it != m_NpcList.end(); it++, index++)
			{
				NpcItem* item = *it;
				if(item != nullptr &&
					map.GetPixelPosition(item->MapX, item->MapY, &itemX, &itemY) &&
					rect.Contains(itemX, itemY))
				{
					m_npcListCtrl->Select(index, isAdd || !isSub || (!isAdd && !isSub));
				}
			}
			ShowNpcSelectionCountMessage();
		}
		else if(m_isObj)
		{
			bool isAdd = wxGetKeyState(SELECTION_ADD_KEY);
			bool isSub = wxGetKeyState(SELECTION_SUB_KEY);
			if(!isAdd && !isSub)
			{
				//clear all selected item
				DeselectAllItem(m_objListCtrl);
			}
			long index = 0;
			for(auto it = m_ObjList.begin(); it != m_ObjList.end(); it++, index++)
			{
				ObjItem* item = *it;
				if(item != nullptr &&
					map.GetPixelPosition(item->MapX, item->MapY, &itemX, &itemY) &&
					rect.Contains(itemX, itemY))
				{
					m_objListCtrl->Select(index, isAdd || !isSub || (!isAdd && !isSub));
				}
			}
			ShowObjSelectionCountMessage();
		}
	}
	m_batchSelectionBeginTile = m_batchSelectionEndTile = INVALID_TILE_POSITION;
}

bool MapTool::IsInMultyPaste()
{
	return (m_isNpc && m_npcsToPasted.getCounts() > 0) ||
		(m_isObj && m_objsToPasted.getCounts() > 0);
}


//////////////////////////////
///////////////////////////////
//NpcItemEditDialog

void NpcItemEditDialog::InitFromNpcItem(NpcItem *item)
{
    if(item == NULL) return;

    m_Name->ChangeValue(item->Name);
    m_ShowName->ChangeValue(item->ShowName);
    if(item->Kind != -1)
    {
        int sel = item->Kind;
        switch(sel)
        {
        case 4:
            sel = 3;
            break;
        case 5:
            sel = 2;
            break;
        case 6:
            sel = 4;
            break;
        case 7:
            sel = 5;
            break;
        }
        m_Kind->SetSelection(sel);
    }
    if(item->Relation != -1)
        m_Relation->SetSelection(item->Relation);
    if(item->PathFinder != -1)
        m_PathFinder->ChangeValue(wxString::Format(wxT("%d"), item->PathFinder));
    if(item->State != -1)
        m_State->ChangeValue(wxString::Format(wxT("%d"), item->State));
    if(item->VisionRadius != -1)
        m_VisionRadius->ChangeValue(wxString::Format(wxT("%d"), item->VisionRadius));
    if(item->DialogRadius != -1)
        m_DialogRadius->ChangeValue(wxString::Format(wxT("%d"), item->DialogRadius));
    if(item->AttackRadius != -1)
        m_AttackRadius->ChangeValue(wxString::Format(wxT("%d"), item->AttackRadius));
    if(item->Dir != -1)
        m_Dir->SetSelection(item->Dir);
    if(item->Lum != -1)
        m_Lum->ChangeValue(wxString::Format(wxT("%d"), item->Lum));
    if(item->Action != -1)
        m_Action->SetSelection(item->Action);
    if(item->WalkSpeed != -1)
        m_WalkSpeed->ChangeValue(wxString::Format(wxT("%d"), item->WalkSpeed));


    if(item->Evade != -1)
        m_Evade->ChangeValue(wxString::Format(wxT("%d"), item->Evade));
    if(item->Attack != -1)
        m_Attack->ChangeValue(wxString::Format(wxT("%d"), item->Attack));
    if(item->AttackLevel != -1)
        m_AttackLevel->ChangeValue(wxString::Format(wxT("%d"), item->AttackLevel));
    if(item->Defend != -1)
        m_Defend->ChangeValue(wxString::Format(wxT("%d"), item->Defend));
    if(item->Exp != -1)
        m_Exp->ChangeValue(wxString::Format(wxT("%d"), item->Exp));
    if(item->LevelUpExp != -1)
        m_LevelUpExp->ChangeValue(wxString::Format(wxT("%d"), item->LevelUpExp));
    if(item->Level != -1)
        m_Level->ChangeValue(wxString::Format(wxT("%d"), item->Level));
    if(item->Life != -1)
        m_Life->ChangeValue(wxString::Format(wxT("%d"), item->Life));
    if(item->LifeMax != -1)
        m_LifeMax->ChangeValue(wxString::Format(wxT("%d"), item->LifeMax));
    if(item->Thew != -1)
        m_Thew->ChangeValue(wxString::Format(wxT("%d"), item->Thew));
    if(item->ThewMax != -1)
        m_ThewMax->ChangeValue(wxString::Format(wxT("%d"), item->ThewMax));
    if(item->Mana != -1)
        m_Mana->ChangeValue(wxString::Format(wxT("%d"), item->Mana));
    if(item->ManaMax != -1)
        m_ManaMax->ChangeValue(wxString::Format(wxT("%d"), item->ManaMax));


    if(item->ExpBonus != -1)
        m_ExpBonus->ChangeValue(wxString::Format(wxT("%d"), item->ExpBonus));
    if(item->Idle != -1)
        m_Idle->ChangeValue(wxString::Format(wxT("%d"), item->Idle));

    if(!item->NpcIni.IsEmpty())
    {
        m_NpcIni->SetLabel(item->NpcIni);
        m_NpcIni->SetToolTip(item->NpcIni);
    }
    if(!item->BodyIni.IsEmpty())
    {
        m_BodyIni->SetLabel(item->BodyIni);
        m_BodyIni->SetToolTip(item->BodyIni);
    }
    if(!item->FlyIni.IsEmpty())
    {
        m_FlyIni->SetLabel(item->FlyIni);
        m_FlyIni->SetToolTip(item->FlyIni);
    }
    if(!item->FlyIni2.IsEmpty())
    {
        m_FlyIni2->SetLabel(item->FlyIni2);
        m_FlyIni2->SetToolTip(item->FlyIni2);
    }
    if(!item->ScriptFile.IsEmpty())
    {
        m_ScriptFile->SetLabel(item->ScriptFile);
        m_ScriptFile->SetToolTip(item->ScriptFile);
    }
    if(!item->DeathScript.IsEmpty())
    {
        m_DeathScript->SetLabel(item->DeathScript);
        m_DeathScript->SetToolTip(item->DeathScript);
    }
    if(!item->FixedPos.IsEmpty())
        m_FixedPos->SetValue(item->FixedPos);
}
void NpcItemEditDialog::AssignToNpcItem(NpcItem *item, bool onlySetted)
{
    if(item == NULL) return;
    wxString value;
    long n_val;

    value = m_Name->GetValue();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->Name = value;

    value = m_ShowName->GetValue();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->ShowName = value;

    n_val = m_Kind->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    else
    {
        switch(n_val)
        {
        case 2:
            n_val = 5;
            break;
        case 3:
            n_val = 4;
            break;
        case 4:
            n_val = 6;
            break;
        case 5:
            n_val = 7;
            break;
        }
    }
    if((onlySetted && n_val != -1) || !onlySetted) item->Kind = n_val;

    n_val = m_Relation->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Relation = n_val;

    value = m_PathFinder->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->PathFinder = n_val;

    value = m_State->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->State = n_val;

    value = m_VisionRadius->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->VisionRadius = n_val;

    value = m_DialogRadius->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->DialogRadius = n_val;

    value = m_AttackRadius->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->AttackRadius = n_val;

    n_val = m_Dir->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Dir = n_val;

    value = m_Lum->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Lum = n_val;

    n_val = m_Action->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Action = n_val;

    value = m_WalkSpeed->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->WalkSpeed = n_val;

    value = m_Evade->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Evade = n_val;

    value = m_Attack->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Attack = n_val;

    value = m_AttackLevel->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->AttackLevel = n_val;

    value = m_Defend->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Defend = n_val;

    value = m_Exp->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Exp = n_val;

    value = m_LevelUpExp->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->LevelUpExp = n_val;

    value = m_Level->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Level = n_val;

    value = m_Life->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Life = n_val;

    value = m_LifeMax->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->LifeMax = n_val;

    value = m_Thew->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Thew = n_val;

    value = m_ThewMax->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->ThewMax = n_val;

    value = m_Mana->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Mana = n_val;

    value = m_ManaMax->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->ManaMax = n_val;

    value = m_ExpBonus->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->ExpBonus = n_val;

    value = m_Idle->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Idle = n_val;


    value = m_NpcIni->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted)
    {
        item->NpcIni = value;
        FindAndBufferAsf(exepath, value, wxT("[Stand]"), &(item->NpcStand), m_NpcAsfImgList);
    }
    value = m_BodyIni->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->BodyIni = value;
    value = m_FlyIni->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->FlyIni = value;
    value = m_FlyIni2->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->FlyIni2 = value;
    value = m_ScriptFile->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->ScriptFile = value;
    value = m_DeathScript->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->DeathScript = value;
    value = m_FixedPos->GetValue();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->FixedPos = value;
}

void ObjItemEditDialog::InitFromObjItem(ObjItem *item)
{
    if(item == NULL) return;

    m_ObjName->ChangeValue(item->ObjName);
    if(item->Kind != -1)
        m_Kind->SetSelection(item->Kind);
    if(item->Dir != -1)
        m_Dir->SetSelection(item->Dir);
    if(item->Damage != -1)
        m_Damage->ChangeValue(wxString::Format(wxT("%d"), item->Damage));
    if(item->Frame != -1)
        m_Frame->ChangeValue(wxString::Format(wxT("%d"), item->Frame));
    if(item->Height != -1)
        m_Height->ChangeValue(wxString::Format(wxT("%d"), item->Height));
    if(item->Lum != -1)
        m_Lum->ChangeValue(wxString::Format(wxT("%d"), item->Lum));
    if(item->OffX != -1)
        m_OffX->ChangeValue(wxString::Format(wxT("%d"), item->OffX));
    if(item->OffY != -1)
        m_OffY->ChangeValue(wxString::Format(wxT("%d"), item->OffY));

    if(!item->ObjFile.IsEmpty())
    {
        m_ObjFile->SetLabel(item->ObjFile);
        m_ObjFile->SetToolTip(item->ObjFile);
    }
    if(!item->ScriptFile.IsEmpty())
    {
        m_ScriptFile->SetLabel(item->ScriptFile);
        m_ScriptFile->SetToolTip(item->ScriptFile);
    }
    if(!item->WavFile.IsEmpty())
    {
        m_WavFile->SetLabel(item->WavFile);
        m_WavFile->SetToolTip(item->WavFile);
    }
}
void ObjItemEditDialog::AssignToObjItem(ObjItem *item, bool onlySetted)
{
    if(item == NULL) return;

    wxString value;
    long n_val;

    value = m_ObjName->GetValue();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->ObjName = value;

    n_val = m_Kind->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Kind = n_val;

    n_val = m_Dir->GetSelection();
    if(n_val == wxNOT_FOUND) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Dir = n_val;

    value = m_Damage->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Damage = n_val;

    value = m_Frame->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Frame = n_val;

    value = m_Height->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Height = n_val;

    value = m_Lum->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->Lum = n_val;

    value = m_OffX->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->OffX = n_val;

    value = m_OffY->GetValue();
    if(!value.ToLong(&n_val)) n_val = -1;
    if((onlySetted && n_val != -1) || !onlySetted) item->OffY = n_val;

    value = m_ObjFile->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted)
    {
        item->ObjFile = value;
        FindAndBufferAsf(exepath, value, wxT("[Common]"), &(item->ObjCommon), m_ObjAsfImgList);
    }

    value = m_ScriptFile->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->ScriptFile = value;

    value = m_WavFile->GetLabel();
    if((onlySetted && !value.IsEmpty()) || !onlySetted) item->WavFile = value;
}

void MapTool::OnMapViewMenu(wxCommandEvent& event)
{
    m_popupMenuShowed = false;
    switch(event.GetId())
    {
    case MYID_MAPVIEW_COPY:
        OnMapViewCopy(event);
        break;
    case MYID_MAPVIEW_CUT:
        OnMapViewCut(event);
        break;
    case MYID_MAPVIEW_PASTE:
        OnMapViewPaste(event);
        break;
    case MYID_MAPVIEW_DETIAL:
        OnMapViewDetail(event);
        break;
    case MYID_MAPVIEW_DELETE:
        OnMapViewDelete(event);
        break;
    case MYID_MAPVIEW_BATEDIT:
        OnMapViewBatEdit(event);
        break;
	case MYID_MAPVIEW_CLEAR_SELECTION:
		OnMapViewClearSelection(event);
		break;
    }
}

void MapTool::OnMapViewCopy(wxCommandEvent& event)
{
    if(m_isNpc)
    {
    	m_clipBoard.ClearNpcs();
    	if(m_isBatch)
		{
            const auto &items = GetAllSelectedItems(m_npcListCtrl);
            for(auto it = items.begin(); it != items.end(); it++)
			{
				NpcItem *item = m_NpcList.GetItem((int)*it);
				if(item)
				{
					m_clipBoard.Add(item);
				}
			}
		}
		else
		{
			m_clipBoard.Add(m_selectedNpcItemByRightUp);
		}

    }
    else if(m_isObj)
    {
    	m_clipBoard.ClearObjs();
    	if(m_isBatch)
		{
			const auto& items = GetAllSelectedItems(m_objListCtrl);
			for(auto it = items.begin(); it != items.end(); it++)
			{
				ObjItem *item = m_ObjList.GetItem((int)*it);
				if(item)
				{
					m_clipBoard.Add(item);
				}
			}
		}
		else
		{
			m_clipBoard.Add(m_selectedObjItemByRightUp);
		}
    }
}

void MapTool::OnMapViewCut(wxCommandEvent& event)
{
    if(m_isNpc)
    {
    	m_clipBoard.ClearNpcs();
    	if(m_isBatch)
		{
			const auto &items = GetAllSelectedItems(m_npcListCtrl);
            for(auto it = items.begin(); it != items.end(); it++)
			{
				NpcItem *item = m_NpcList.GetItem((int)*it);
				if(item)
				{
					m_clipBoard.Add(item);
				}
			}

			MTC_Delete_Npcs *cmd = new MTC_Delete_Npcs();
			for(auto it = m_clipBoard.GetNpcItems().begin(); it != m_clipBoard.GetNpcItems().end(); it++)
			{
				long index = 0;
				NpcItem *item = m_NpcList.GetItem(it->MapX, it->MapY, &index);
				if(item)
				{
					cmd->DeleteItem(index, item);
				}
				m_NpcList.DeleteItem(it->MapX, it->MapY);
			}
			m_commandProcessor.Store(cmd);
		}
		else
		{
			if(m_selectedNpcItemByRightUp)
			{
				m_clipBoard.Add(m_selectedNpcItemByRightUp);

				long index = 0;
				auto item = m_NpcList.GetItem(m_selectedNpcItemByRightUp->MapX, m_selectedNpcItemByRightUp->MapY, &index);
				if(item)
				{
					MTC_Delete_Npc *cmd = new MTC_Delete_Npc(index);
					cmd->DeletedItem(item);
					m_commandProcessor.Store(cmd);
				}
				m_NpcList.DeleteItem(m_selectedNpcItemByRightUp->MapX, m_selectedNpcItemByRightUp->MapY);
			}
		}
		RefreshNpcList();
    }
    else if(m_isObj)
    {
    	m_clipBoard.ClearObjs();
    	if(m_isBatch)
		{
			const auto &items = GetAllSelectedItems(m_objListCtrl);
			for(auto it = items.begin(); it != items.end(); it++)
			{
				ObjItem *item = m_ObjList.GetItem((int)*it);
				if(item)
				{
					m_clipBoard.Add(item);
				}
			}

			MTC_Delete_Objs *cmd = new MTC_Delete_Objs();
			for(auto it = m_clipBoard.GetObjItems().begin(); it != m_clipBoard.GetObjItems().end(); it++)
			{
				long index = 0;
				ObjItem *item = m_ObjList.GetItem(it->MapX, it->MapY, &index);
				if(item)
				{
					cmd->DeleteItem(index, item);
				}
				m_ObjList.DeleteItem(it->MapX, it->MapY);
			}
			m_commandProcessor.Store(cmd);
		}
		else
		{
			if(m_selectedObjItemByRightUp)
			{
				m_clipBoard.Add(m_selectedObjItemByRightUp);

				long index = 0;
				auto item = m_ObjList.GetItem(m_selectedObjItemByRightUp->MapX, m_selectedObjItemByRightUp->MapY, &index);
				if(item)
				{
					MTC_Delete_Obj *cmd = new MTC_Delete_Obj(index);
					cmd->DeletedItem(item);
					m_commandProcessor.Store(cmd);
				}

				m_ObjList.DeleteItem(m_selectedObjItemByRightUp->MapX, m_selectedObjItemByRightUp->MapY);
			}
		}
		RefreshObjList();
    }
}

void MapTool::OnMapViewPaste(wxCommandEvent& event)
{
    if(m_isNpc)
    {
    	if(m_clipBoard.NpcCount() == 1)
		{
			NpcItem *data = &(m_clipBoard.GetNpcItems().front());
			if(!data) return;
			NpcItem *item = new NpcItem;
			item->CopyFrom(data);
			item->MapX = m_CurTileX;
			item->MapY = m_CurTileY;
			FindAndBufferAsf(exepath, item, nullptr, m_NpcAsfImgList);
			if(!AddItem(item))
			{
				delete item;
			}
		}
		else if(m_clipBoard.NpcCount() > 1)
		{
			m_npcsToPasted.Clear();
			for(auto it = m_clipBoard.GetNpcItems().begin(); it != m_clipBoard.GetNpcItems().end(); it++)
			{
				NpcItem *item = new NpcItem;
				item->CopyFrom(&(*it));
				FindAndBufferAsf(exepath, item, nullptr, m_NpcAsfImgList);
				m_npcsToPasted.AddItem(item);
			}
			m_tilePositionOffsetNpc = m_clipBoard.GetMinNpcTilePosition();
		}
    }
    else if(m_isObj)
    {
    	if(m_clipBoard.ObjCount() == 1)
		{
			ObjItem *data = &(m_clipBoard.GetObjItems().front());
			if(!data) return;
			ObjItem *item = new ObjItem;
			item->CopyFrom(data);
			item->MapX = m_CurTileX;
			item->MapY = m_CurTileY;
			FindAndBufferAsf(exepath, nullptr, item, m_ObjAsfImgList);
			if(!AddItem(item))
			{
				delete item;
			}
		}
        else if(m_clipBoard.ObjCount() > 1)
		{
			m_objsToPasted.Clear();
			for(auto it = m_clipBoard.GetObjItems().begin(); it != m_clipBoard.GetObjItems().end(); it++)
			{
				ObjItem *item = new ObjItem;
				item->CopyFrom(&(*it));
				FindAndBufferAsf(exepath, nullptr, item, m_ObjAsfImgList);
				m_objsToPasted.AddItem(item);
			}
			m_tilePositionOffsetObj = m_clipBoard.GetMinObjTilePostion();
		}
    }
}

void MapTool::OnMapViewDetail(wxCommandEvent& event)
{
    if(m_isNpc)
    {
        if(m_selectedNpcItemByRightUp)
        {
            long index = 0;
            NpcItem *item = m_NpcList.GetItem(m_selectedNpcItemByRightUp->MapX,
                                              m_selectedNpcItemByRightUp->MapY,
                                              &index);
            ShowNpcItemEditor(item, index);
        }
    }
    else if(m_isObj)
    {
        if(m_selectedObjItemByRightUp)
        {
            long index = 0;
            ObjItem *item = m_ObjList.GetItem(m_selectedObjItemByRightUp->MapX,
                                              m_selectedObjItemByRightUp->MapY,
                                              &index);
            ShowObjItemEditor(item, index);
        }
    }
}

void MapTool::OnMapViewDelete(wxCommandEvent& event)
{
    if(!PromptDelection())
    {
        return;
    }
    if(m_isNpc)
    {
    	if(m_isBatch)
		{
			NpcList tempList;
			const auto &items = GetAllSelectedItems(m_npcListCtrl);
            for(auto it = items.begin(); it != items.end(); it++)
			{
				NpcItem *item = m_NpcList.GetItem((int)*it);
				if(item)
				{
					tempList.AddItem(item);
				}
			}

			MTC_Delete_Npcs *cmd = new MTC_Delete_Npcs();
			for(auto item : tempList)
			{
				cmd->DeleteItem(m_NpcList.GetIndex(item), item);
				m_NpcList.DeleteItem(item->MapX, item->MapY);
			}
			m_commandProcessor.Store(cmd);

			tempList.ClearWithoutFreeData();
			RefreshNpcList();
		}
		else
		{
			if(m_selectedNpcItemByRightUp)
			{
				long index = 0;
				NpcItem *item = m_NpcList.GetItem(m_selectedNpcItemByRightUp->MapX,
												  m_selectedNpcItemByRightUp->MapY,
												  &index);
				if(item)
				{
					MTC_Delete_Npc *cmd = new MTC_Delete_Npc(index);
					cmd->DeletedItem(item);
					m_commandProcessor.Store(cmd);

					DeleteNpcItem((int)index);
					RefreshNpcList();
				}
			}
		}
    }
    else if(m_isObj)
    {
    	if(m_isBatch)
		{
			ObjList tempList;
			const auto &items = GetAllSelectedItems(m_objListCtrl);
            for(auto it = items.begin(); it != items.end(); it++)
			{
				ObjItem *item = m_ObjList.GetItem((int)*it);
				if(item)
				{
					tempList.AddItem(item);
				}
			}

			MTC_Delete_Objs *cmd = new MTC_Delete_Objs();
			for(auto item : tempList)
			{
				cmd->DeleteItem(m_ObjList.GetIndex(item), item);
				m_ObjList.DeleteItem(item->MapX, item->MapY);
			}
			m_commandProcessor.Store(cmd);
			tempList.ClearWithoutFreeData();
			RefreshObjList();
		}
		else
		{
			if(m_selectedObjItemByRightUp)
			{
				long index = 0;
				ObjItem *item = m_ObjList.GetItem(m_selectedObjItemByRightUp->MapX,
												  m_selectedObjItemByRightUp->MapY,
												  &index);
				if(item)
				{
					MTC_Delete_Obj *cmd = new MTC_Delete_Obj(index);
					cmd->DeletedItem(item);
					m_commandProcessor.Store(cmd);

					DeleteObjItem((int)index);
					RefreshObjList();
				}
			}
		}
    }
}

void MapTool::OnMapViewBatEdit(wxCommandEvent& event)
{
    ShowBatEditDialog();
}

void MapTool::OnMapViewClearSelection(wxCommandEvent& event)
{
	if(m_isNpc)
	{
		DeselectAllItem(m_npcListCtrl);
	}
	else if(m_isObj)
	{
		DeselectAllItem(m_objListCtrl);
	}
}

void DeselectAllItem(wxListView* listCtrl)
{
	if(listCtrl)
	{
		int count = listCtrl->GetItemCount();
		for(int i = 0; i < count; i++)
		{
			listCtrl->Select(i, false);
		}
	}
}

bool MapTool::PromptDelection()
{
    return(wxMessageBox(wxT("ȷ��ɾ����"),
                        wxMessageBoxCaptionStr,
                        wxYES_NO | wxCENTER | wxICON_QUESTION) == wxYES);
}

void MapTool::MapPositionToScreenPositon(int tileX, int tileY, int* posX, int* posY)
{
    if(!posX || !posY) return;
    int relateX = 0, relateY = 0;
    Map::GetTileCenterPixelPosition(&relateX, &relateY, tileX, tileY);
    int screenX,screenY;
    m_MapView->GetScreenPosition(&screenX, &screenY);
    relateX -= m_ViewBeginx;
    relateY -= m_ViewBeginy;
    *posX = screenX + relateX;
    *posY = screenY + relateY;
}

bool MapTool::IsItemSelectedAtTile(int tileX, int tileY)
{
    long index;
    if(m_isNpc)
    {
        if(m_NpcList.GetItem(tileX, tileY, &index))
        {
            return m_npcListCtrl->IsSelected(index);
        }
    }
    else if(m_isObj)
    {
        if(m_ObjList.GetItem(tileX, tileY, &index))
        {
            return m_objListCtrl->IsSelected(index);
        }
    }
    return false;
}
