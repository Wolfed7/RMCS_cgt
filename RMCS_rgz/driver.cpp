#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>


const int ERR_NO_TIME = -1;
const int ERR_NO_CACHE = -2;
const int ERR_NO_TIME_AND_CACHE = -3;

const wchar_t DYNAMIC_LIB1[] = L"info.dll";
const wchar_t CLASS_NAME[] = L"RM_RGZ";

const wchar_t MSG1[] = L"Текущая минута системного времени: %i.\n";
const wchar_t MSG2[] = L"Размер процессорного КЭШа третьего уровня: %i килобайт.\n";
const wchar_t ERR_MSG1[] = L"Не удалось определить текущую минуту.\n";
const wchar_t ERR_MSG2[] = L"Не удалось найти КЭШ третьего уровня.\n";

char Info[BUFSIZ];
wchar_t Mesage[BUFSIZ];
wchar_t Message_about_cahce[BUFSIZ];
int cacheSize;
int systemMinute;

// Функция, запускаемая в рамках созданного потока,
// выполняет действия, определённые вариантом задания.
DWORD WINAPI ThreadFunc(void *)
{
   // Подключение динамической библиотеки.
   typedef int (*ImportFunction)(char *);
   ImportFunction DLLInfo;
   HINSTANCE hInstanceLib = LoadLibrary(DYNAMIC_LIB1);
   DLLInfo = (ImportFunction)GetProcAddress(hInstanceLib, "Information");
   int retCode = DLLInfo(Info);
   FreeLibrary(hInstanceLib);

   // Интерпретация полученной информации о минуте и КЭШе.
   memcpy(&systemMinute, Info, sizeof(WORD));
   memcpy(&cacheSize, Info + sizeof(WORD), sizeof(int));

   if (retCode == 0)
   {
      wsprintf(Mesage, MSG1, systemMinute);
      wsprintf(Message_about_cahce, MSG2, cacheSize);
   }

   if (retCode == ERR_NO_TIME)
   {
      wsprintf(Mesage, ERR_MSG1);
      wsprintf(Message_about_cahce, MSG2, cacheSize);
   }

   if (retCode == ERR_NO_CACHE)
   {
      wsprintf(Mesage, MSG1, systemMinute);
      wsprintf(Message_about_cahce, ERR_MSG2);
   }

   if (retCode == ERR_NO_TIME_AND_CACHE)
   {
      wsprintf(Mesage, ERR_MSG1);
      wsprintf(Message_about_cahce, ERR_MSG2);
   }

   return 0;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_CREATE:
         HANDLE hThread;
         DWORD IDThread;
         // Запуск функции в рамках созданного потока.
         hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &IDThread);
         if (hThread != 0)
         {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
         }
         break;

      case WM_DESTROY:
         // Зыкрытие окна приложения.
         PostQuitMessage(0);
         return 0;

      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hwnd, &ps);

         // Создадим жирный шрифт семейства consolas 18го размера.
         HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
               DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, 
               CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("consolas"));

         SelectObject(hdc, hFont);
         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

         // Вывод минуты системного времени 
         // и размера КЭШа в окно.
         TextOut(hdc, 15, 15, Mesage, wcslen(Mesage));
         TextOut(hdc, 15, 45, Message_about_cahce, wcslen(Message_about_cahce));

         EndPaint(hwnd, &ps);
      }
      return 0;

   }
   return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
   WNDCLASS wndclass = {};
   wndclass.lpfnWndProc = WinProc;       // Название процедуры обработки сообщений.
   wndclass.hInstance = hInst;           // Дескриптор экземпляра.
   wndclass.lpszClassName = CLASS_NAME;  // Имя класса окна.

   // Регистрируем класс окна.
   RegisterClass(&wndclass);

   // Теперь нужно создать окно на базе зарегистрированного класса.
   HWND hwnd = CreateWindowEx(
      0,                           // Расширенные стили окна.
      CLASS_NAME,                  // Класс окна.
      L"РГЗ по дисциплине УРВС, ПМ - 02, Щукин",   // Заголовок окна.
      WS_OVERLAPPEDWINDOW,         // Стиль окна.
      CW_USEDEFAULT,          // Горизонтальная позиция создаваемого окна.
      CW_USEDEFAULT,          // Вертикальная позиция.
      520,                    // Ширина окна.
      130,                    // Высота окна.
      NULL,       // Дескриптор родительного окна.   
      NULL,       // Дескриптор оконного меню.
      hInst,      // Дескриптор экземпляра.
      NULL        // Дополнительные данные приложения.
   );

   if (hwnd == NULL)
      return 1;

   // Отображение окна.
   ShowWindow(hwnd, cmdshow);

   // Запустим цикл обработки сообщений, вызывающий процедуру обрабоки WinProc.
   MSG msg = { };
   while (GetMessage(&msg, NULL, 0, 0) > 0) // Получение сообщения.
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
   return msg.wParam;
}