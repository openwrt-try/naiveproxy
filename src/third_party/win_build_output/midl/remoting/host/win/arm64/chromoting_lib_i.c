

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for gen/remoting/host/win/chromoting_lib.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=ARM64 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IRdpDesktopSessionEventHandler,0xb59b96da,0x83cb,0x40ee,0x9b,0x91,0xc3,0x77,0x40,0x0f,0xc3,0xe3);


MIDL_DEFINE_GUID(IID, IID_IRdpDesktopSession,0x6a7699f0,0xee43,0x43e7,0xaa,0x30,0xa6,0x73,0x8f,0x9b,0xd4,0x70);


MIDL_DEFINE_GUID(IID, LIBID_ChromotingLib,0xb6396c45,0xb0cc,0x456b,0x9f,0x49,0xf1,0x29,0x64,0xee,0x6d,0xf4);


MIDL_DEFINE_GUID(CLSID, CLSID_RdpDesktopSession,0x7bf898db,0xce6c,0x57a3,0xa4,0x73,0x3a,0xa2,0x1b,0xf8,0xc4,0xc4);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



