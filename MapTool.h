#ifndef MAPTOOL_H
#define MAPTOOL_H

#include <MapFrameBase.h>
#include "Map.h"
#include "Npc.hpp"
#include "AsfDecode.hpp"

#include "wx/dcbuffer.h"
#include "wx/filedlg.h"
#include "wx/stdpaths.h"
#include "wx/msgdlg.h"

class MapTool : public MapFrameBase
{
public:
    MapTool(wxWindow* parent);
    virtual ~MapTool();
protected:
private:

	enum MYID {
		ID_SHOWNPCKEY = 10000,
		ID_SHOWOBJKEY,
		ID_ATTRIBUTELISTITEM,
		ID_DELETELISTITEM
	};

	void OnClose( wxCloseEvent& event );
    void OpenMap(wxCommandEvent& event);
    void SaveToPNG(wxCommandEvent& event);
    void OnLayer1( wxCommandEvent& event ) ;
    void OnLayer2( wxCommandEvent& event );
    void OnLayer3( wxCommandEvent& event );
    void OnTrap( wxCommandEvent& event ) ;
    void OnBarrer( wxCommandEvent& event ) ;
    void OnMapDraw( wxPaintEvent& event ) ;
    void OnLayerTransparent( wxCommandEvent& event );

    void OnMapUp( wxCommandEvent& event ) ;
    void OnMapDown( wxCommandEvent& event ) ;
    void OnMapLeft( wxCommandEvent& event ) ;
    void OnMapRight( wxCommandEvent& event ) ;
    void OnMapViewMouseLeftDown( wxMouseEvent& event );
    void OnMapViewMouseLeftUp( wxMouseEvent& event );

    void OnMouseMove( wxMouseEvent& event );

    void OnDrawMapControl( wxPaintEvent& event );
    void OnMapCtrlMouseMotion( wxMouseEvent& event );

    void OnNpcMode( wxCommandEvent& event )
    {
        m_isNpc = true;
        m_isObj = false;
        m_ToolBarEdit->ToggleTool(ID_NPCMODE, true);
        m_ToolBarEdit->ToggleTool(ID_OBJMODE, false);
        RedrawMapView();
    }
    void OnObjMode( wxCommandEvent& event )
    {
        m_isNpc = false;
        m_isObj = true;
        m_ToolBarEdit->ToggleTool(ID_NPCMODE, false);
        m_ToolBarEdit->ToggleTool(ID_OBJMODE, true);
        RedrawMapView();
    }

    void ShowNpcItemEditor(long npcitemidx);
    void ShowObjItemEditor(long objitemidx);

    //NPC
    void OnLoadCharater( wxCommandEvent& event );
    void OnPlaceMode( wxCommandEvent& event ) ;
    void OnCharacterDirection( wxCommandEvent& event );
    void OnImportNpcFile( wxCommandEvent& event );
    void OnOutputNpcFile( wxCommandEvent& event );
    void OnShowNpcCounts( wxCommandEvent& event )
    {
        wxMessageBox(wxString::Format(wxT("NPC ������Count = %d"), m_NpcList.getCounts()),
                     wxT("��Ϣ"));
    }
    void OnClearNpc( wxCommandEvent& event )
    {
        m_NpcList.Clear();
        FreeAsfImgList(m_NpcAsfImgList);
        RefreshNpcList();
    }

    //OBJ
    void OnLoadObject( wxCommandEvent& event );
    void OnObjectDirection( wxCommandEvent& event )
    {
        OnCharacterDirection(event);
    }
    void OnImportObjFile( wxCommandEvent& event );
    void OnOutputObjFile( wxCommandEvent& event );
    void OnShowObjCount( wxCommandEvent& event )
    {
        wxMessageBox(wxString::Format(wxT("OBJ ������Count = %d"), m_ObjList.getCounts()),
                     wxT("��Ϣ"));
    }
    void OnClearObj( wxCommandEvent& event )
    {
        m_ObjList.Clear();
        FreeAsfImgList(m_ObjAsfImgList);
        RefreshObjList();
    }


    void OnDeleteMode( wxCommandEvent& event );

    void OnMoveMode( wxCommandEvent& event );

    void OnEditAttributeMode( wxCommandEvent& event );

    void OnShowNpcCheck( wxCommandEvent& event ) { RedrawMapView(); }
    void OnShowObjCheck( wxCommandEvent& event ) { RedrawMapView(); }
    void OnShowNpcCheckKey( wxCommandEvent& event)
    {
    	m_ToolBarEdit->ToggleTool(ID_SHOWNPC, !m_ToolBarEdit->GetToolState(ID_SHOWNPC));
    	RedrawMapView();
    }
    void OnShowObjCheckKey( wxCommandEvent& event)
    {
    	m_ToolBarEdit->ToggleTool(ID_SHOWOBJ, !m_ToolBarEdit->GetToolState(ID_SHOWOBJ));
    	RedrawMapView();
    }

    //ListData
    void OnListCtrlLeftDClick( wxMouseEvent& event );
    void OnListCtrlRightDown( wxListEvent& event );
    void OnListItemSelected( wxListEvent& event );
    void RefreshNpcList();
    void RefreshObjList();
    void ShowTile(int tileX, int tileY);
    void DeleteListItem( wxCommandEvent& event);
    void AttributeListItem( wxCommandEvent& event);
    void ShowAttributeListItem();
    void UpdateListItem(int index, int listType = NPCLIST);
    void UpdateListItem(NpcItem *item);
    void UpdateListItem(ObjItem *item);
    enum  {NPCLIST, OBJLIST};
    int m_curList, m_curListIndex;
    bool m_placeModeNotDraw;

    //currentView: if true dc is the current mapview client,
    // else dc is the entire map
    void DrawRectangle(long col, long row, wxDC &dc, bool currentView = true);
    void DrawTile(long col,
                  long row,
                  wxDC &dc,
                  NpcItem *npcitem,
                  ObjItem *objitem = NULL,
                  bool currentView = true);
    void DrawObjsNpcs(wxDC &dc, bool currentView = true);
    void DrawObjsNpcsPosition(wxDC &dc, bool currentView = true);
    void DrawNpcPosition(wxDC &dc, bool currentView = true);
    void DrawObjPostion(wxDC &dc, bool currentView = true);

    //if getImg is true return an image, else return NULL
    wxImage* ReadMap(bool getImg = false);
    void RedrawMapView();
    void CheckMapViewBeginPosition();

    wxBitmap m_MapBitmap;
    Map map;
    int m_ViewBeginx, m_ViewBeginy;
    //Current tile under mouse
    int m_CurTileX, m_CurTileY;
    wxString exepath, m_MapFileName;
    char m_NpcCurrentDir;
    bool m_isPlaceMode, m_isDeleteMode, m_isEditAttribute, m_isMoveMode;

    //Npc obj list
    NpcItem m_PlaceNpcData, *m_MoveNpcItem;
    ObjItem m_PlaceObjData, *m_MoveObjItem;
    NpcList m_NpcList;
    ObjList m_ObjList;
    AsfImgList *m_NpcAsfImgList, *m_ObjAsfImgList;
    wxString m_NpcIniFilePath, m_ObjIniFilePath;
    bool m_isObj, m_isNpc;

    //File dialog path
    wxString m_NpcObjPath;

    DECLARE_EVENT_TABLE()
};

class NpcItemEditDialog: public NpcItemEditDialogBase
{
public:
    NpcItemEditDialog(wxWindow *parent,
                      const wxString mapname,
                      AsfImgList *list,
                      NpcItem *item)
        :NpcItemEditDialogBase(parent)
    {
        exepath = wxStandardPaths::Get().GetExecutablePath();
        exepath = wxFileName::FileName(exepath).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
        m_mapName = mapname;
        m_NpcAsfImgList = list;
        m_item = item;

        INI_MASK = wxT("INI�ļ�(*.ini)|*.ini");
        TXT_MASK = wxT("TXT�ļ�(*.txt)|*.txt");
        INI_MESSGEG = wxT("ѡ��һ��INI�ļ�");
        STYLE = wxFD_OPEN;
    }
    virtual ~NpcItemEditDialog() {}
    void InitFromNpcItem(NpcItem *item);
    void AssignToNpcItem(NpcItem *item);
private:

    void OnOk( wxCommandEvent& event )
    {
        EndModal(wxID_OK);
    }
    void OnCancle( wxCommandEvent& event )
    {
        EndModal(wxID_CANCEL);
    }

    void OnNpcIni( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             INI_MESSGEG,
                             exepath + wxT("ini\\npcres\\"),
                             wxT(""),
                             INI_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_NpcIni->SetLabel(filedlg.GetFilename());
            m_NpcIni->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnNpcIniEdit( wxCommandEvent& event )
    {
        wxString path = m_NpcIni->GetLabel();
        if(!path.IsEmpty()) path = wxT("ini\\npcres\\") + path;
        OpenFile(path);
    }
    void OnBodyIni( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             INI_MESSGEG,
                             exepath + wxT("ini\\obj\\"),
                             wxT(""),
                             INI_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_BodyIni->SetLabel(filedlg.GetFilename());
            m_BodyIni->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnBodyIniEdit( wxCommandEvent& event )
    {
        wxString path = m_BodyIni->GetLabel();
        if(!path.IsEmpty()) path = wxT("ini\\obj\\") + path;
        OpenFile(path);
    }
    void OnFlyIni( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             INI_MESSGEG,
                             exepath + wxT("ini\\magic\\"),
                             wxT(""),
                             INI_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_FlyIni->SetLabel(filedlg.GetFilename());
            m_FlyIni->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnFlyIniEdit( wxCommandEvent& event )
    {
        wxString path = m_FlyIni->GetLabel();
        if(!path.IsEmpty()) path = wxT("ini\\magic\\") + path;
        OpenFile(path);
    }
    void OnFlyIni2( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             INI_MESSGEG,
                             exepath + wxT("ini\\magic\\"),
                             wxT(""),
                             INI_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_FlyIni2->SetLabel(filedlg.GetFilename());
            m_FlyIni2->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnFlyIni2Edit( wxCommandEvent& event )
    {
        wxString path = m_FlyIni2->GetLabel();
        if(!path.IsEmpty()) path = wxT("ini\\magic\\") + path;
        OpenFile(path);
    }
    void OnScriptFile( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("ѡ��һ���ű��ļ�"),
                             exepath + wxT("script\\map\\") + m_mapName + wxT("\\"),
                             wxT(""),
                             TXT_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_ScriptFile->SetLabel(filedlg.GetFilename());
            m_ScriptFile->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnScriptFileEdit( wxCommandEvent& event )
    {
        wxString path = m_ScriptFile->GetLabel();
        if(!path.IsEmpty()) path = wxT("script\\map\\") + m_mapName + wxT("\\")+ path;
        OpenFile(path);
    }
    void OnDeathScript( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("ѡ��һ���ű��ļ�"),
                             exepath + wxT("script\\map\\") + m_mapName + wxT("\\"),
                             wxT(""),
                             TXT_MASK,
                             STYLE);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_DeathScript->SetLabel(filedlg.GetFilename());
            m_DeathScript->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnDeathScriptEdit( wxCommandEvent& event )
    {
        wxString path = m_DeathScript->GetLabel();
        if(!path.IsEmpty()) path = wxT("script\\map\\") + m_mapName + wxT("\\")+ path;
        OpenFile(path);
    }

    void OnBodyIniClear( wxMouseEvent& event )
    {
        m_BodyIni->SetLabel(wxT(""));
        m_BodyIni->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnFlyIniClear( wxMouseEvent& event )
    {
        m_FlyIni->SetLabel(wxT(""));
        m_FlyIni->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnFlyIni2Clear( wxMouseEvent& event )
    {
        m_FlyIni2->SetLabel(wxT(""));
        m_FlyIni2->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnScriptFileClear( wxMouseEvent& event )
    {
        m_ScriptFile->SetLabel(wxT(""));
        m_ScriptFile->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnDeathScriptClear( wxMouseEvent& event )
    {
        m_DeathScript->SetLabel(wxT(""));
        m_DeathScript->SetToolTip(wxT("���ѡ���Ҽ����"));
    }


    void OnSaveNpcIniFile( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("����INI"),
                             exepath + wxT("ini\\npc\\"),
                             wxT(""),
                             INI_MASK,
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if(filedlg.ShowModal() == wxID_OK)
        {
            NpcItem item;
            InitNpcItem(&item);
            AssignToNpcItem(&item);
            item.MapX = m_item->MapX;
            item.MapY = m_item->MapY;
            SaveIni(filedlg.GetPath(), &item);
        }
    }



    void OpenFile(wxString relatePath)
    {
        if(relatePath.IsEmpty())
        {
            wxMessageBox(wxT("����ѡ���ļ�"), wxT("��Ϣ"));
            return;
        }
        if(wxFileName::FileExists(exepath + relatePath))
        {
            wxExecute(wxT("explorer \"") + exepath + relatePath + wxT("\""));
        }
        else
        {
            wxMessageBox(relatePath + wxT("  �ļ�������"), wxT("��Ϣ"));
            return;
        }
    }



    wxString INI_MASK,INI_MESSGEG, TXT_MASK;
    long STYLE;
    wxString exepath, m_mapName;
    AsfImgList *m_NpcAsfImgList;
    NpcItem *m_item;
};

class ObjItemEditDialog: public ObjItemEditDialogBase
{
public:

    ObjItemEditDialog(wxWindow *parent,
                      const wxString mapname,
                      AsfImgList *list,
                      ObjItem *objitem)
        :ObjItemEditDialogBase(parent)
    {
        exepath = wxStandardPaths::Get().GetExecutablePath();
        exepath = wxFileName::FileName(exepath).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
        m_mapName = mapname;
        m_ObjAsfImgList = list;
        m_item = objitem;
    }
    virtual ~ObjItemEditDialog() {}
    void InitFromObjItem(ObjItem *item);
    void AssignToObjItem(ObjItem *item);
private:

    void OnObjFile( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("ѡ��һ��INI�ļ�"),
                             exepath + wxT("ini\\objres\\"),
                             wxT(""),
                             wxT("INI�ļ�(*.ini)|*.ini"),
                             wxFD_OPEN);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_ObjFile->SetLabel(filedlg.GetFilename());
            m_ObjFile->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnEditObjFile( wxCommandEvent& event )
    {
        wxString path = m_ObjFile->GetLabel();
        if(!path.IsEmpty()) path = wxT("ini\\objres\\") + path;
        OpenFile(path);
    }
    void OnScriptFile( wxCommandEvent& event )
    {
         wxFileDialog filedlg(this,
                             wxT("ѡ��һ���ű��ļ�"),
                             exepath + wxT("script\\common\\"),
                             wxT(""),
                             wxT("TXT�ļ�(*.txt)|*.txt"),
                             wxFD_OPEN);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_ScriptFile->SetLabel(filedlg.GetFilename());
            m_ScriptFile->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnClearScriptFile( wxMouseEvent& event )
    {
        m_ScriptFile->SetLabel(wxT(""));
        m_ScriptFile->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnEditScriptFile( wxCommandEvent& event )
    {
        wxString path = m_ScriptFile->GetLabel();
        if(!path.IsEmpty())
        {
            if(wxFileName::FileExists(exepath + wxT("script\\common\\") + path))
                path = wxT("script\\common\\") + path;
            else
                path = wxT("script\\map\\") + m_mapName + wxT("\\") + path;
        }
        OpenFile(path);
    }
    void OnWavFile( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("ѡ�������ļ�"),
                             exepath + wxT("sound\\"),
                             wxT(""),
                             wxT("WAV�ļ�(*.wav)|*.wav"),
                             wxFD_OPEN);

        if(filedlg.ShowModal() == wxID_OK)
        {
            m_WavFile->SetLabel(filedlg.GetFilename());
            m_WavFile->SetToolTip(filedlg.GetFilename());
        }
    }
    void OnClearWavFile( wxMouseEvent& event )
    {
        m_WavFile->SetLabel(wxT(""));
        m_WavFile->SetToolTip(wxT("���ѡ���Ҽ����"));
    }
    void OnOpenWavFile( wxCommandEvent& event )
    {
        wxString path = m_WavFile->GetLabel();
        if(!path.IsEmpty()) path = wxT("sound\\") + path;
        OpenFile(path);
    }
    void OnOk( wxCommandEvent& event )
    {
        EndModal(wxID_OK);
    }
    void OnCancle( wxCommandEvent& event )
    {
        EndModal(wxID_CANCEL);
    }
    void OnSaveObjIniFile( wxCommandEvent& event )
    {
        wxFileDialog filedlg(this,
                             wxT("����INI"),
                             exepath + wxT("ini\\obj\\"),
                             wxT(""),
                             wxT("INI�ļ�(*.ini)|*.ini"),
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if(filedlg.ShowModal() == wxID_OK)
        {
            ObjItem item;
            InitObjItem(&item);
            AssignToObjItem(&item);
            item.MapX = m_item->MapX;
            item.MapY = m_item->MapY;
            SaveIni(filedlg.GetPath(), NULL, &item);
        }
    }

    void OpenFile(wxString relatePath)
    {
        if(relatePath.IsEmpty())
        {
            wxMessageBox(wxT("����ѡ���ļ�"), wxT("��Ϣ"));
            return;
        }
        if(wxFileName::FileExists(exepath + relatePath))
        {
            wxExecute(wxT("explorer \"") + exepath + relatePath + wxT("\""));
        }
        else
        {
            wxMessageBox(relatePath + wxT("  �ļ�������"), wxT("��Ϣ"));
            return;
        }
    }

    wxString exepath, m_mapName;
    AsfImgList *m_ObjAsfImgList;
    ObjItem *m_item;
};

#endif // MAPTOOL_H
