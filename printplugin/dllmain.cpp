
#include <windows.h>



#define memAlloc(size)   HeapAlloc(memHeap,HEAP_ZERO_MEMORY,size)
#define memFree(blk)     HeapFree(memHeap,0,blk)
#define memSize(blk)     HeapSize(memHeap,0,blk)



HANDLE     memHeap;
HANDLE     printHandle;
HWND       MainWindow;
LPDEVMODE  dOut;
LONG       wOut;
HDC        DC;
HDC        MemDC;
int        xRes;
int        yRes;
int        xPix;
int        yPix;
int        xOff;
int        yOff;
int        xMil;
int        yMil;
HFONT      ofont;
HFONT      ufont;
HPEN       open;
HPEN       upen;
TCHAR      name[130];



BOOL  memStart(void) {
   ULONG lfh;

   memHeap = HeapCreate(0, 0, 0);
   lfh = 2;
   HeapSetInformation(memHeap, HeapCompatibilityInformation, &lfh, sizeof(ULONG));
   return memHeap!=0;

}



void  memStop(void) {

   HeapDestroy(memHeap);

}





LRESULT CALLBACK MainWindowProcess(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {

   return DefWindowProc(wnd, msg, wParam, lParam);

}




void CreateMainWindow(HINSTANCE instance) {
   WNDCLASSA  w;

   w.style = 0;
   w.lpfnWndProc = DefWindowProc;
   w.cbClsExtra = 0;
   w.cbWndExtra = 0;
   w.hInstance = instance;
   w.hIcon = 0;
   w.hCursor = 0;
   w.hbrBackground = 0;
   w.lpszClassName = "UnityPrintPlugIn";
   w.lpszMenuName = 0;
   RegisterClassA(&w);

   MainWindow = CreateWindowA("UnityPrintPlugIn", 0, 0, 0, 0, 0, 0, 0, 0, instance, 0);
   
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
	switch (ul_reason_for_call){
	case DLL_PROCESS_ATTACH:
      memStart();
      CreateMainWindow(hModule);
      break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
      break;
	case DLL_PROCESS_DETACH:
		memStop();
      break;
	}
	return TRUE;
}



extern "C" {



   __declspec(dllexport) void SetPrinterName(LPCWSTR prn) {
      lstrcpy(name,prn);
   }



   __declspec(dllexport) BOOL StartPrinter(LPCSTR title,int or,int width,int height) {
     
      DOCINFOA di;
      bool     defprn = true;
     
      if (name[0] != 0) {
         DWORD             sa, rb;
         LPPRINTER_INFO_1  pr;

         EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, 0, 0, &sa, &rb);
         pr = (LPPRINTER_INFO_1)malloc(sa);
         if (pr == 0) {
            defprn = true;
         } else {
            EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, (LPBYTE)pr, sa, &sa, &rb);
            bool rv = false;
            for (DWORD i = 0; i < rb; i++) {
               if (lstrcmpi(name, pr[i].pName) == 0) {
                  defprn = false;
                  break;
               }
            }
            free(pr);
         }
      }

      if (defprn) {
         DWORD   lname = 128;
         GetDefaultPrinter(name, &lname);
      }

      ofont = 0;
      ufont = 0;
      open  = 0;
      upen  = 0;

      if (OpenPrinter(name, &printHandle, NULL)) {
         wOut = DocumentProperties(MainWindow, printHandle, name, 0, 0, 0);
         dOut = (LPDEVMODE)memAlloc(wOut);
         if (dOut) {
            if (DocumentProperties(MainWindow, printHandle, name, dOut, 0, DM_OUT_BUFFER) != 0) {
               dOut->dmPaperSize = 0;
              // dOut->dmFields    = DM_PAPERSIZE;
               if (width != 0) {
                  dOut->dmPaperWidth = width;
                //  dOut->dmFields    |= DM_PAPERWIDTH;
               }
               if(height!=0){
                  dOut->dmPaperLength = height;
                //  dOut->dmFields     |= DM_PAPERLENGTH;
               }
               if (or!=0) {
                  if (or==1) {
                     dOut->dmOrientation = DMORIENT_PORTRAIT;
                  }
                  else {
                     dOut->dmOrientation = DMORIENT_LANDSCAPE;
                  }
                  //  dOut->dmFields |= DM_ORIENTATION;
               }
               DocumentProperties(MainWindow, printHandle, name, dOut, dOut, DM_IN_BUFFER | DM_OUT_BUFFER);
               
               DC = CreateDC(L"WINSPOOL", name, 0, dOut);
               if (!GetDeviceCaps(DC, RASTERCAPS & RC_BITBLT)) {
                  memFree(dOut);
                  DeleteDC(DC);
                  ClosePrinter(printHandle);
                  return FALSE;
               }
               di.cbSize = sizeof(DOCINFO);
               di.lpszDocName = title;
               di.lpszOutput = 0;
               di.lpszDatatype = 0;
               di.fwType = 0;
               if (StartDocA(DC, &di) == SP_ERROR) {
                  memFree(dOut);
                  DeleteDC(DC);
                  ClosePrinter(printHandle);
                  return FALSE;
               }
               xRes = GetDeviceCaps(DC, HORZRES);
               yRes = GetDeviceCaps(DC, VERTRES);
               xPix = GetDeviceCaps(DC, PHYSICALWIDTH);
               yPix = GetDeviceCaps(DC, PHYSICALHEIGHT);
               xOff = GetDeviceCaps(DC, PHYSICALOFFSETX);
               yOff = GetDeviceCaps(DC, PHYSICALOFFSETY);
               xMil = GetDeviceCaps(DC, HORZSIZE);
               yMil = GetDeviceCaps(DC, VERTSIZE);

               MemDC = CreateCompatibleDC(DC);
               if (StartPage(DC)>0) return TRUE;

               return FALSE;
            }
         }

      }
      return FALSE;

   }



   void PrepareGraphics() {

      HDC HdnDC = CreateDC(L"DISPLAY", 0, 0, 0);
      if (HdnDC) {
         MemDC = CreateCompatibleDC(HdnDC);
         DeleteDC(HdnDC);
      } else {
         MemDC = 0;
      }
   }



   __declspec(dllexport) int GetPrinterNames(int size,wchar_t *names) {
      DWORD             sa, rb;
      LPPRINTER_INFO_1  pr;

      EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, 0, 0, &sa, &rb);
      int err = GetLastError();
      if(err!=0 && err!=122){
         return -err;
      }
      pr = (LPPRINTER_INFO_1) malloc(sa);
      if (pr == 0) {
         return -2;
      }

      EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, (LPBYTE) pr, sa, &sa, &rb);
      err = GetLastError();
      if (err != 0) {
         return -err;
      }

      DWORD ct = 0;

      if (names != NULL) {
         for (DWORD i = 0; i < rb; i++) {
            lstrcpy(names + ct, pr[i].pName);
            ct += lstrlen(pr[i].pName);
            names[ct++] = '\r';
         }
         ct++;
      }  else {
         for (DWORD i = 0; i < rb; i++) {
            ct += (lstrlen(pr[i].pName) + 1);
         }
         ct++;
      }

      free(pr);

      return ct * sizeof(wchar_t);

   }



   _declspec(dllexport) bool IsPrinterName(LPWSTR name) {
      DWORD             sa, rb;
      LPPRINTER_INFO_1  pr;

      EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, 0, 0, &sa, &rb);
      pr = (LPPRINTER_INFO_1)malloc(sa);
      if (pr == 0) {
         return false;
      }

      EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 0, 1, (LPBYTE)pr, sa, &sa, &rb);

      bool rv = false;
      for (DWORD i = 0; i < rb;i++) {
         if (lstrcmpi(name, pr[i].pName) == 0) {
            rv = true;
            break;
         }
      }
      free(pr);
      return rv;

   }



   __declspec(dllexport) void StopPrinter() {

      if(ofont)  SelectObject(DC, ofont);
      if(ufont)  DeleteObject(ufont);
      if(open)   SelectObject(DC, open);
      if(upen)   DeleteObject(upen);

      memFree(dOut);
      if (EndPage(DC) > 0) EndDoc(DC);
      DeleteDC(DC);
      ClosePrinter(printHandle);

   }



   __declspec(dllexport) void PrinterNewPage() {

      EndPage(DC);
      StartPage(DC);

   }



   __declspec(dllexport) int PaperWidth() {
      return xRes;
   }



   __declspec(dllexport) int PaperHeight() {
      return yRes;
   }



   __declspec(dllexport) void PrintText(wchar_t *text, int x, int y) {
      RECT rect;

      rect.left   = x;
      rect.top    = y;
      rect.right  = x;
      rect.bottom = y;
      int count   = lstrlen(text);
      DrawText(DC, text, count, &rect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);

      DrawText(DC, text, count, &rect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

   }



   __declspec(dllexport) int PrintTextWidth(wchar_t *text) {
      RECT rect;

      rect.left   = 0;
      rect.top    = 0;
      rect.right  = 0;
      rect.bottom = 0;
      int count = lstrlen(text);
      DrawText(DC, text, count, &rect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
      return rect.right;

   }



   __declspec(dllexport) int PrintTextHeight(wchar_t *text) {
      RECT rect;

      rect.left = 0;
      rect.top = 0;
      rect.right = 0;
      rect.bottom = 0;
      int count = lstrlen(text);
      DrawText(DC, text, count, &rect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
      return rect.bottom;

   }




   __declspec(dllexport) void PrintDrawText(int x,int y,int cx,int cy,int align, wchar_t *txt) {
      TEXTMETRIC  tm;
      RECT        r, c;
      int        sl, fm, ln, ly, fx, xs, mw, dw, mi;

      r.top = y;
      r.left = x;
      r.bottom = cy;
      r.right = cx;

      sl = (WORD) lstrlen(txt);
      switch (align) {
      case 0:
         fm = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
         break;

      case 1:
         fm = DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX;
         break;

      case 2:
         fm = DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_NOPREFIX;
         break;

      case 3:
         fm = DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS;
         ln = (WORD)(cx - x);
         ly = (WORD)(cy - y);
         c.left = 0;
         c.top = 0;
         c.right = ln;
         c.bottom = 0;
         DrawText(DC, txt, sl, &c, fm | DT_CALCRECT);
         if (ly>c.bottom) r.top += (ly - c.bottom) / 2;
         break;

      case 4:
         fm = DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS;
         ln = (WORD)(cx - x);
         ly = (WORD)(cy - y);
         c.left = 0;
         c.top = 0;
         c.right = ln;
         c.bottom = 0;
         DrawText(DC, txt, sl, &c, fm | DT_CALCRECT);
         if (ly>c.bottom) r.top += (ly - c.bottom) / 2;
         break;

      case 5:
         fm = DT_RIGHT | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS;
         ln = (WORD)(cx - x);
         ly = (WORD)(cy - y);
         c.left = 0;
         c.top = 0;
         c.right = ln;
         c.bottom = 0;
         DrawText(DC, txt, sl, &c, fm | DT_CALCRECT);
         if (ly>c.bottom) r.top += (ly - c.bottom) / 2;
         break;

      case 6:
         fx = (WORD)(cx - x);
         c.left = 0;
         c.top = 0;
         c.right = fx;
         c.bottom = 0;
         DrawText(DC, txt, sl, &c, DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
         if (c.right>fx) {
            xs = 0;
         } else {
            mw = (WORD)(fx - fx / 4);
            if (c.right<mw) {
               dw = (WORD)(mw - c.right);
               mw = (WORD)c.right;
               c.right = fx;
               SetTextCharacterExtra(DC, 1);
               DrawText(DC, txt, sl, &c, DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
               SetTextCharacterExtra(DC, 0);
               if (c.right>mw) {
                  mw = (WORD)(dw / (c.right - mw));
               }
               GetTextMetrics(DC, &tm);
               mi = (WORD)(tm.tmAveCharWidth*3);
               if (mw>mi) {
                  xs = mi;
               }
               else {
                  xs = mw;
               }
            }
            else {
               xs = 0;
            }
         }
         ly = (WORD)(cy - y);
         if (ly>c.bottom) r.top += (ly - c.bottom) / 2;
         SetTextCharacterExtra(DC, xs);
         DrawText(DC, txt, sl, &r, DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_WORDBREAK);
         SetTextCharacterExtra(DC, 0);
         return;

      default:
         fm = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
         break;

      }

      DrawText(DC, txt, sl, &r, fm);

   }



   __declspec(dllexport) void SetPrinterFont(LPCSTR name, int sz,int wg,bool it) {

      int hs = sz * 33 / 16;
      int hw = hs * 10 / 27;

      if (ofont) {
         SelectObject(DC, ofont);
         ofont = 0;
      }
      if (ufont) {
         DeleteObject(ufont);
      }

      switch (wg) {
      case 1:
         wg = FW_BOLD;
         break;
      case 2:
         wg = FW_BLACK;
         break;
      case 3:
         wg = FW_EXTRABOLD;
         break;
      case 4:
         wg = FW_EXTRALIGHT;
         break;
      case 5:
         wg = FW_HEAVY;
         break;
      default:
         wg = FW_NORMAL;
         break;
      }

      if(ufont) DeleteObject(ufont);
      ufont = CreateFontA(hs, hw, 0, 0,wg, it, 0, 0, 0, 0, 0, 0, 0, name);
      if(ofont){
         SelectObject(DC, ufont);
      }else{
         ofont = (HFONT)SelectObject(DC, ufont);
      }

   }



   __declspec(dllexport) bool SetPrinterOrientation(int or) {
      if (or==1) {
         dOut->dmOrientation = DMORIENT_PORTRAIT;
      }else {
         dOut->dmOrientation = DMORIENT_LANDSCAPE;
      }
    //  dOut->dmFields |= DM_ORIENTATION;
      if(ResetDC(DC, dOut) != 0) {
         xRes = GetDeviceCaps(DC, HORZRES);
         yRes = GetDeviceCaps(DC, VERTRES);
         return TRUE;
      }
      return FALSE;      
   }



   __declspec(dllexport) void  CreatePrinterPen(int s,int w,int r,int g,int b) {

      int style;
      switch (s) {
      case 1:
         style = PS_DASH;
         break;
      case 2:
         style = PS_DOT;
         break;
      case 3:
         style = PS_DASHDOT;
         break;
      default:
         style = PS_SOLID;
         break;
      }

      if (upen) DeleteObject(upen);
      upen = CreatePen(style, w,RGB(r,g,b));
      if (open) {
         SelectObject(DC, upen);
      }else{
         open = (HPEN)SelectObject(DC, upen);
      }

   }

   

   __declspec(dllexport) void  PrintMoveTo(int x, int y) {

      MoveToEx(DC, x, y, 0);

   }



   __declspec(dllexport) void  PrintLineTo(int x, int y) {

      LineTo(DC, x, y);

   }



   __declspec(dllexport) void PrintDrawRect(int x1, int y1, int x2, int y2) {

      MoveToEx(DC, x1, y1, 0);
      LineTo(DC, x2, y1);
      LineTo(DC, x2, y2);
      LineTo(DC, x1, y2);
      LineTo(DC, x1, y1);

      // This may seem useless, however it avoids round corners when pens are 2 o more pixels wide.
      MoveToEx(DC, x2, y1, 0);
      LineTo(DC, x1, y1);
      LineTo(DC, x1, y2);

   }



   __declspec(dllexport) void PrintDrawLine(int x1, int y1,int x2, int y2) {

      MoveToEx(DC, x1, y1, 0);
      LineTo(DC, x2, y2);

   }

   

   __declspec(dllexport) void  PrintFillRect(int x, int y, int cx, int cy, int r,int g,int b) {
      RECT    rect;
      HBRUSH  br;

      rect.left = x;
      rect.top = y;
      rect.right = cx;
      rect.bottom = cy;

      br = CreateSolidBrush(RGB(r,g,b));
      FillRect(DC, &rect, br);
      DeleteObject(br);

   }

   

   __declspec(dllexport) int PrintBitmap(int x, int y, int cx, int cy, byte* binfo, bool adj, int rop) {
      BITMAP              bmi;
      WORD                dx, dy, tx, ty, px, py;
     // HDC                 dc;
      LPBITMAPFILEHEADER  bmf;
      LPBITMAPINFOHEADER  bih;
      HBITMAP             bmp;

      bmf = (LPBITMAPFILEHEADER) binfo;
      bih = (LPBITMAPINFOHEADER)(bmf + 1);
    //  dc = CreateDC(L"DISPLAY", 0, 0, 0);
    //  if (dc) {
         bmp = CreateDIBitmap(DC, bih, CBM_INIT, (LPSTR)bmf + bmf->bfOffBits, (LPBITMAPINFO)bih, DIB_RGB_COLORS);
         if (bmp == 0) {
            return GetLastError();
         }
      /*   DeleteDC(dc);
      } else {
         return GetLastError();
      }*/
      
      switch (rop) {
      case 1:
         rop = SRCAND;
         break;
      case 2:
         rop = SRCPAINT;
         break;
      default:
         rop = SRCCOPY;
         break;
      }
      
      GetObject(bmp, sizeof(BITMAP), &bmi);
      SelectObject(DC, bmp);
      if (adj) {
         if (bmi.bmWidth>0 && bmi.bmHeight>0) {
            dx = (WORD)((cx * 100) / bmi.bmWidth);
            dy = (WORD)((cy * 100) / bmi.bmHeight);
            dx = (WORD)(dx<dy ? dx : dy);
            tx = (WORD)((bmi.bmWidth*dx) / 100);
            ty = (WORD)((bmi.bmHeight*dx) / 100);
            px = (WORD)(x + ((cx - tx) / 2));
            py = (WORD)(y + ((cy - ty) / 2));
            StretchBlt(DC, px, py, tx, ty, DC, 0, 0, bmi.bmWidth, bmi.bmHeight, rop);
         } else {
            return -2;
         }
      } else {
         StretchBlt(DC, x, y, cx, cy, DC, 0, 0, bmi.bmWidth, bmi.bmHeight, rop);
      }
      
      return GetLastError();

   }



   __declspec(dllexport) int PrintLoadBitmap(int x, int y, int cx, int cy, wchar_t *name, bool adj, DWORD rop) {
      BITMAP              bmi;
      WORD                dx, dy, tx, ty, px, py;
      LPBITMAPFILEHEADER  bmf;
      LPBITMAPINFOHEADER  bih;
      HBITMAP             bmp;
      DWORD               rb;
      HDC                 dc;
      HDC                 mdc;
      
      switch (rop) {
      case 1:
         rop = SRCAND;
         break;
      case 2:
         rop = SRCPAINT;
         break;
      default:
         rop = SRCCOPY;
         break;
      }

      bmp = (HBITMAP) LoadImage(0, name, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
      if (bmp == 0) {
         return GetLastError();
      }
       
      GetObject(bmp, sizeof(BITMAP), &bmi);
      HBITMAP obmp = (HBITMAP) SelectObject(MemDC, bmp);
      if (adj) {
         if (bmi.bmWidth>0 && bmi.bmHeight>0) {
            dx = (WORD)((cx * 100) / bmi.bmWidth);
            dy = (WORD)((cy * 100) / bmi.bmHeight);
            dx = (WORD)(dx<dy ? dx : dy);
            tx = (WORD)((bmi.bmWidth*dx) / 100);
            ty = (WORD)((bmi.bmHeight*dx) / 100);
            px = (WORD)(x + ((cx - tx) / 2));
            py = (WORD)(y + ((cy - ty) / 2));
            if (!StretchBlt(DC, px, py, tx, ty, MemDC, 0, 0, bmi.bmWidth, bmi.bmHeight, rop)) {
               return GetLastError();
            }
         }else{
            return -2;
         }
      } else {
         if (!StretchBlt(DC, x, y, cx, cy,MemDC, 0, 0, bmi.bmWidth, bmi.bmHeight, rop)) {
            return GetLastError();
         }
      }

      SelectObject(MemDC, obmp);
      return GetLastError();
            
   }
   
        

}