#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>

int retCode = 0;

// ������������ �������, ����������� ������� IV ������ ���.
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

// �������� ����������� ������������� cpuid �� ������ ����������.
void can_we_use_cpuid(int &no_cpuid)
{
   int retval;
   _asm
   {
      pushfd    ; ��������� EFLAGS � ����
      pop eax   ; ��������� EFLAGS � EAX

      mov edx, eax   ; �������� EFLAGS
      mov ebx, eax   ; ��������� � EBX ��� ������������ ������������

      xor eax, 00200000h   ; ����������� ��� 21
      push eax             ; ����������� ���������� �������� � ����
      popfd                ; ��������� ���������� EAX � EFLAGS
      pushfd               ; EFLAGS �� ������� �����
      pop eax              ; ��������� EFLAGS � EAX

      push edx         
      popfd    ; ������� ����� �� �����

      cmp eax, ebx         ; ��������� 21 - � ��� �� ���������
      jne YES_CPUID        ; ���� ���� ���������, �� CPUID ��������������

      mov eax, 1h
      mov retval, eax

      YES_CPUID:
   }

   no_cpuid = retval;
}

extern "C" _declspec(dllexport) int Information(char *InfoString)
{
   // ������� ��������� ����� �������� GetSystemTime.
   SYSTEMTIME st;
   GetSystemTime(&st);
   // ������� � ����� ���������� ������ ���������� �������.
   memcpy(InfoString, &st.wMinute, sizeof(WORD));

   int no_cpuid = 0;
   can_we_use_cpuid(no_cpuid);

   //CPUID �� �������������� �����������.
   if (no_cpuid == 1)
	  return -2;

   /*
	  ��� ���� ����� ������ ������ ���� �������� ������,
	  ����� ������� ������ ������������� ����������, ��� 
	  ��� �� ���� ������� ������������� ������������ ��������.
   */

   /*
     ������, ������������ ������������� ����������. 
	  �� ����� ������������ ������ ��� �� ��� - ��� Intel � AMD.

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
     � �.�.
   */

   int CPUInfo[4];
   char *manufacturerID = new char[13];
   manufacturerID[12] = '\0';
   asm_cpuid(CPUInfo, 0x0);
   memcpy(manufacturerID, CPUInfo + 1, sizeof(int));
   memcpy(manufacturerID + 4, CPUInfo + 3, sizeof(int));
   memcpy(manufacturerID + 8, CPUInfo + 2, sizeof(int));

   // ��������� ������� ���� �������� ������ ��� ����������� �������� Intel.
   if (strcmp(manufacturerID, "GenuineIntel") == 0)
   {
      for (size_t i = 0; i < 4; i++)
         CPUInfo[i] &= 0x0;
    
      int safetyExit = 100;
      for (int i = 0; i < safetyExit; i++)
      {
         CPUInfo[2] = i;
         asm_cpuid(CPUInfo, 0x4);

         // EAX[4:0] ���������� 0, ���� ����� ������ ���.
         if ((CPUInfo[0] & 0x1f) == 0) // EAX[4:0]
            break;

         // ��� �� �������� ������ ��� �� ���������.
         if ((CPUInfo[0] >> 5 & 0x7) != 3) // EAX[7:5]
            continue;

         int L3CacheSize =
            ((CPUInfo[1] & 0xfff) + 1)         // Line size + 1    EAX[11:0]
            * ((CPUInfo[1] >> 12 & 0x3ff) + 1) // Partitions + 1   EBX[21:12]
            * ((CPUInfo[1] >> 22 & 0x3ff) + 1) // Ways + 1         EBX[31:22]
            * (CPUInfo[2] + 1);                // Sets + 1         ECX[31:0]

         // ����� -> ���������.
         L3CacheSize /= 1024;

         memcpy(InfoString + sizeof(WORD), &L3CacheSize, sizeof(int));
         return 0;
      }

      // ���������� � ���� �� �������.
      return retCode == -1 ? -3 : -2;
   }
   // ��������� ������� ���� �������� ������ ��� ����������� �������� AMD.
   else if (strcmp(manufacturerID, "AuthenticAMD") == 0)
   {
	  asm_cpuid(CPUInfo, 0x80000000); // ����� ������������ �������� ������������ �������.
	  if (CPUInfo[0] < 0x80000006)
	  {
		 // ���� ������� 80000006 ���, �� ������ ���� �� ��������.
		 return retCode == -1 ? -3 : -2;
	  }

	  asm_cpuid(CPUInfo, 0x80000006);
     // ���� 31-18 �������� ���������� � ������� ���� (* 512 ��).
     unsigned int L3CacheSize = (CPUInfo[3] & 0xfffc0000) / 512;
     memcpy((InfoString + sizeof(WORD)), &L3CacheSize, sizeof(int));
   }
   else
   {
	  // �� ������������ ���������� ������ ��������������.
      return retCode == -1 ? -3 : -2;
   }

   return 0;
}