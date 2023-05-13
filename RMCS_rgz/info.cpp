#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>

int retCode = 0;

// Ассемблерная вставка, выполняющая функцию IV уровня РГЗ.
void asm_cpuid(int regs[4], int func)
{
   int ieax = regs[0], iebx = regs[1], iecx = regs[2], iedx = regs[3];
   _asm
   {
      mov ebx, iebx
      mov ecx, iecx
      mov edx, iedx

	   mov eax, func
	   cpuid
	   mov ieax, eax
	   mov iebx, ebx
	   mov iecx, ecx
	   mov iedx, edx
   }
   regs[0] = ieax;
   regs[1] = iebx;
   regs[2] = iecx;
   regs[3] = iedx;
}

// Проверка возможности использования cpuid на данном процессоре.
void can_we_use_cpuid(int &no_cpuid)
{
   int retval;
   _asm
   {
      pushfd    ; Сохранить EFLAGS в стек
      pop eax   ; Поместить EFLAGS в EAX

      mov edx, eax   ; Копируем EFLAGS
      mov ebx, eax   ; Сохранить в EBX для последующего тестирования

      xor eax, 00200000h   ; Переключить бит 21
      push eax             ; Скопировать измененное значение в стек
      popfd                ; Сохранить измененное EAX в EFLAGS
      pushfd               ; EFLAGS на вершину стека
      pop eax              ; Сохранить EFLAGS в EAX

      push edx         
      popfd    ; Вернуть флаги на место

      cmp eax, ebx         ; Проверить 21 - й бит на изменение
      jne YES_CPUID        ; Если есть изменение, то CPUID поддерживается

      mov eax, 1h
      mov retval, eax

      YES_CPUID:
   }

   no_cpuid = retval;
}

extern "C" _declspec(dllexport) int Information(char *InfoString)
{
   // Получим системное время командой GetSystemTime.
   SYSTEMTIME st;
   GetSystemTime(&st);
   // Запишем в буфер полученную минуту системного времени.
   memcpy(InfoString, &st.wMinute, sizeof(WORD));

   int no_cpuid = 0;
   can_we_use_cpuid(no_cpuid);

   //CPUID не поддерживается процессором.
   if (no_cpuid == 1)
	  return -2;

   /*
	  Для того чтобы узнать размер кэша третьего уровня,
	  нужно сначала узнать производителя процессора, так 
	  как от него зависит интерпретация возвращаемых значений.
   */

   /*
     Строки, определяющие производителя процессора. 
	  Мы будем обрабатывать только две из них - для Intel и AMD.

	  "GenuineIntel" - Intel
	  "AuthenticAMD" - AMD
	  "CyrixInstead" - Cyrix
	  "UMC UMC UMC " - UMC
	  "NexGenDriven" - NexGen
	  "CentaurHauls" - Centaur
	  "RiseRiseRise" - Rise Technology
	  "SiS SiS SiS " - SiS
	  "GenuineTMx86" - Transmeta
	  "Geode by NSC" - National Semiconductor
     и т.д.
   */

   int CPUInfo[4];
   char *manufacturerID = new char[13];
   manufacturerID[12] = '\0';
   asm_cpuid(CPUInfo, 0x0);
   memcpy(manufacturerID, CPUInfo + 1, sizeof(int));
   memcpy(manufacturerID + 4, CPUInfo + 3, sizeof(int));
   memcpy(manufacturerID + 8, CPUInfo + 2, sizeof(int));


   // Получение размера КЭШа третьего уровня для процессоров компании Intel.
   if (strcmp(manufacturerID, "GenuineIntel") == 0)
   {
      for (size_t i = 0; i < 4; i++)
         CPUInfo[i] &= 0x0;
    
      int safetyExit = 100;
      for (int i = 0; i < safetyExit; i++)
      {
         CPUInfo[2] = i;
         asm_cpuid(CPUInfo, 0x4);

         FILE *d;
         fopen_s(&d, "debug.dat", "a");
         for (size_t j = 0; j < 4; j++)
            fprintf_s(d, "%x\n", CPUInfo[j]);
         fprintf_s(d, "\n");

         // EAX[4:0] возвращает 0, если КЭШей больше нет.
         if ((CPUInfo[0] & 0x1f) == 0) // EAX[4:0]
            break;

         // КЭШ не третьего уровня нам не интересен.
         if ((CPUInfo[0] & 0xe0) != 3) // EAX[7:5]
            continue;

         int L3CacheSize =
            ((CPUInfo[1] & 0xfff) + 1)         // Line size + 1
            * ((CPUInfo[1] & 0x3ff000) + 1)    // Partitions + 1
            * ((CPUInfo[1] & 0xffc00000) + 1)  // Ways + 1
            * (CPUInfo[2] + 1);                // Sets + 1

         memcpy(InfoString + sizeof(WORD), &L3CacheSize, sizeof(int));
      }

      // Информация о КЭШе не найдена.
      return retCode == -1 ? -3 : -2;
   }
   // Получение размера КЭШа третьего уровня для процессоров компании AMD.
   else if (strcmp(manufacturerID, "AuthenticAMD") == 0)
   {
	  asm_cpuid(CPUInfo, 0x80000000); // Вернёт максимальное значение существующей команды.
	  if (CPUInfo[0] < 0x80000006)
	  {
		 // Если команды 80000006 нет, то размер КЭШа не получить.
		 return retCode == -1 ? -3 : -2;
	  }

	  asm_cpuid(CPUInfo, 0x80000006);
     // Биты 31-18 содержат информацию о размере КЭШа (* 512 кб).
     unsigned int L3CacheSize = (CPUInfo[3] & 0xfffc0000) / 512;
     memcpy((InfoString + sizeof(WORD)), &L3CacheSize, sizeof(int));
   }
   else
   {
	  // Не обрабатываем процессоры других производителей.
      return retCode == -1 ? -3 : -2;
   }

   return 0;
}