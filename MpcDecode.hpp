#ifndef MPCDECODE_HPP_INCLUDED
#define MPCDECODE_HPP_INCLUDED

#include "wx/string.h"
#include "wx/image.h"

#include "mydefs.hpp"

class MpcDecode
{
public:
    enum COLOUR_MODLE {PIC_RGB,PIC_RGBA, PIC_BGRA};
    MpcDecode();
    ~MpcDecode();
    MpcDecode(const wxString MpcFilePath);
    void SetMpcFile(const wxString MpcFilePath);
    wxString GetFilePath();
    bool ReadMpcFile();
    bool ReadMpcFile(const wxString MpcFilePath);
    wxString GetVersionInfo();
    unsigned long GetFramesDataLengthSum();
    unsigned long GetFramesCounts();
    //unsigned long GetFrameDataLength(const unsigned long index);
    long GetGlobleWidth();
    long GetGlobleHeight();
    //long GetFrameWidth(const unsigned long index);
    //long GetFrameHeight(const unsigned long index);
    long GetDirection();
    unsigned long GetColourCounts();
    unsigned long GetInterval();
    long GetBottom();

    //data must be free() if won't use anymore
    Palette_Colour GetTransparentColor();
    unsigned char* GetDecodedFrameData(const unsigned long index, long* Width = NULL, long* Height = NULL,
                                       COLOUR_MODLE mod = PIC_RGB);

    wxImage GetFrameImage(const unsigned long index);


    Palette PaletteData;


private:
    void Init();

    MpcFileHead FileHead;
    wxString FilePath;
    unsigned long FrameDataBegPos;

};

#endif // MPCDECODE_HPP_INCLUDED
