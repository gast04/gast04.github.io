__int64 __fastcall anti_debug_54DDA0C(__int64 *a1)
{
  int8x16_t v1; // q1
  __int64 (__fastcall *v3)(char *); // x2
  __int64 fd; // x0
  unsigned int v5; // w21
  int read_bytes; // w0
  __int64 v7; // x1
  __int64 (__fastcall *v8)(char *, char *); // x4
  char *v9; // x0
  __int64 v10; // x2
  int v12; // [xsp+44h] [xbp-23Ch] BYREF
  char v13[8]; // [xsp+48h] [xbp-238h] BYREF
  char tracerpid_str[7]; // [xsp+50h] [xbp-230h] BYREF
  char v15[8]; // [xsp+57h] [xbp-229h] BYREF
  char proc_self_status[24]; // [xsp+60h] [xbp-220h] BYREF
  char buffer[512]; // [xsp+78h] [xbp-208h] BYREF

  v1.n128_u64[0] = 0x1B1B1B1B1B1B1B1BLL;
  v1.n128_u64[1] = 0x1B1B1B1B1B1B1B1BLL;
  strcpy(proc_self_status, "4kitx4h~w}4hozons");
  v3 = *(__int64 (__fastcall **)(char *))(a1[2] + 128);
  v12 = 0;
  *(int8x16_t *)proc_self_status = veorq_s8(*(int8x16_t *)proc_self_status, v1);
  fd = v3(proc_self_status);                    // openat call
  v5 = fd;
  if ( (_DWORD)fd != -1 )
  {

    // read syscall
    read_bytes = (*(__int64 (__fastcall **)(__int64, char *, __int64))(a1[2] + 96))(fd, buffer, 512LL);
    if ( read_bytes > 0 )
    {
      v7 = a1[2];
      tracerpid_str[6] = 'P';
      buffer[read_bytes - 1] = 0;
      v8 = *(__int64 (__fastcall **)(char *, char *))(v7 + 56);
      strcpy(v15, "id");
      qmemcpy(tracerpid_str, "Tracer", 6);
      v9 = (char *)v8(&buffer[40], tracerpid_str);// strparsing
      if ( v9 )
      {
        v10 = a1[2];
        strcpy(v13, "%*s%d");
        (*(void (__fastcall **)(char *, char *, int *))(v10 + 120))(v9, v13, &v12);// sscanf
      }
    }
    (*(void (__fastcall **)(_QWORD))(a1[2] + 136))(v5);// close
  }
  return 0LL;
}
