#ifndef UNICODE
#define UNICODE
#endif 


#include <iostream>
#include <windows.h>

const wchar_t DYNAMIC_LIB1[] = L"info.dll";
const wchar_t CLASS_NAME[] = L"Resourse Management RGZ";

char Info[BUFSIZ];


TCHAR computerName[BUFSIZ];
DWORD computerNameLen;

DWORD L1Size;
TCHAR L1Info[BUFSIZ];

// Выполняет действия, определённые вариантом задания.
DWORD WINAPI ThreadFunc(void *)
{
   typedef int (*ImportFunction)(char *);
   ImportFunction DLLInfo;
   HINSTANCE hInstanceLib = LoadLibrary(DYNAMIC_LIB1);

   DLLInfo = (ImportFunction)GetProcAddress(hInstanceLib, "Information");
   
   computerNameLen = BUFSIZ;
   //sprintf_s(Info, "%i", computerNameLen);
   int res = DLLInfo(Info);
   FreeLibrary(hInstanceLib);


   sscanf_s(Info, "%i", &computerNameLen);
   wsprintf(computerName, L"Имя компьютера: %ls", (TCHAR *)(Info + 4));

   if (res)
   {
      swscanf_s((LPWSTR)(Info + 4) + computerNameLen, L"%i", &L1Size);
      wsprintf(L1Info, L"Размер кэша данных первого уровня: %i Кбайт", L1Size);
   }
   else
      wsprintf(L1Info, L"Не удалось определить размер кэша данных первого уровня.");
   return 0;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   HANDLE hThread;
   DWORD IDThread;



   switch (uMsg)
   {
      case WM_CREATE:
         hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &IDThread);
         if (hThread != 0)
         {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
         }
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;

      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hwnd, &ps);

         // All painting occurs here, between BeginPaint and EndPaint.

         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

         TextOut(hdc, 10, 10, (LPCWSTR)Info, strlen(Info));

         EndPaint(hwnd, &ps);
      }
      return 0;

   }
   return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
   WNDCLASS wndclass = {};
   wndclass.lpfnWndProc = WinProc;
   wndclass.hInstance = hInst;
   wndclass.lpszClassName = CLASS_NAME;


   RegisterClass(&wndclass);


   /* Создание окна на базе созданного класса */
   HWND hwnd = CreateWindowEx(
      0,                              // Optional window styles.
      CLASS_NAME,                     // Window class
      L"Learn to Program Windows",    // Window text
      WS_OVERLAPPEDWINDOW,            // Window style
      // Size and position
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      NULL,       // Parent window    
      NULL,       // Menu
      hInst,      // Instance handle
      NULL        // Additional application data
   );



   if (hwnd == NULL)
      return 0;

   /* Отображение окна */
   ShowWindow(hwnd, cmdshow);

   /* Цикл обработки сообщений */
   MSG msg = { };
   while (GetMessage(&msg, NULL, 0, 0) > 0) // Получение сообщения
   {
      TranslateMessage(&msg); // Преобразование виртуальных кодов клавиш в ASCII-значения
      DispatchMessage(&msg); // Посылка сообщения в нужную оконную процедуру
   }
   return msg.wParam;


   //return 0;
}