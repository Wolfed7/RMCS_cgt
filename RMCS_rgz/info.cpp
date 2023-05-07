#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>

int retCode = 0;

// Ассемблерная вставка, выполняющая функцию IV уровня РГЗ.
void asm_cpuid(int regs[4], int func)
{
   int ieax, iebx, iecx, iedx;
   _asm
   {
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
   int ret;
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
      mov ret, eax

      YES_CPUID:
   }

   no_cpuid = ret;
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
	  // Дескрипторы, относящиеся к КЭШу третьего уровня.
	  int return_descriptors[] = 
	  { 
		  0x22, 0x23, 0x25, 0x29, 0x46, 0x47,
        0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0xD0,
        0xD1, 0xD2, 0xD6, 0xD7, 0xD8, 0xDC,
        0xDD, 0xDE, 0xE2, 0xE3, 0xE4, 0xEA,
        0xEB, 0xEC
	  };

     // Размер КЭШа третьего уровня в килобайтах в соответствии с дескрипторами.
	  int cache_sizes[] = 
     {
        512, 1024, 2048, 4096, 4096, 8192,
        4096, 6144, 8192, 12288, 16384, 512,
        1024, 2048, 1024, 2048, 4096, 1536,
        3072, 6144, 2048, 4096, 8192, 12288,
        18432, 24576
     };

     for (size_t i = 0; i < 4; i++)
        CPUInfo[i] &= 0x0;

     asm_cpuid(CPUInfo, 0x2);

     for (size_t i = 0; i < 4; i++)
     {
        for (size_t j = 0; j < 26; j++)
        {
           if ((CPUInfo[i] >> 24 & 0xff) == return_descriptors[j])
           {
              memcpy(InfoString + sizeof(WORD), &cache_sizes[j], sizeof(int));
              return retCode;
           }

           if ((CPUInfo[i] >> 16 & 0xff) == return_descriptors[j])
           {
              memcpy(InfoString + sizeof(WORD), &cache_sizes[j], sizeof(int));
              return retCode;
           }

           if ((CPUInfo[i] >> 8 & 0xff) == return_descriptors[j])
           {
              memcpy(InfoString + sizeof(WORD), &cache_sizes[j], sizeof(int));
              return retCode;
           }

           if ((CPUInfo[i] & 0xff) == return_descriptors[j])
           {
              memcpy(InfoString + sizeof(WORD), &cache_sizes[j], sizeof(int));
              return retCode;
           }
        }

        // Информация о КЭШе не найдена.
        return retCode == -1 ? -3 : -2;
     }
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