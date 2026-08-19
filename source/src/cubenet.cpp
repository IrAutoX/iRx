#include "cube.h"

#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>

#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"advapi32.lib")

static char httpResult[65536];

char updateVersion[64];
char updateTitle[128];
char updateSha256[128];
char updateFile[256];
char updateTarget[256];

vector<char *> updateChanges;

bool http_get(const char *url)
{
    httpResult[0] = 0;

    wchar_t wurl[2048];

    MultiByteToWideChar(
        CP_UTF8,
        0,
        url,
        -1,
        wurl,
        2048
    );

    URL_COMPONENTS uc;
    memset(&uc,0,sizeof(uc));

    wchar_t host[256];
    wchar_t path[2048];

    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;

    if(!WinHttpCrackUrl(wurl,0,0,&uc))
        return false;


    HINTERNET s = WinHttpOpen(
        L"iRx",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        NULL,
        NULL,
        0
    );

    if(!s)
        return false;


    HINTERNET c = WinHttpConnect(
        s,
        host,
        uc.nPort,
        0
    );

    if(!c)
    {
        WinHttpCloseHandle(s);
        return false;
    }


    HINTERNET r = WinHttpOpenRequest(
        c,
        L"GET",
        path,
        NULL,
        NULL,
        NULL,
        uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0
    );


    if(!r)
    {
        WinHttpCloseHandle(c);
        WinHttpCloseHandle(s);
        return false;
    }


    bool ok=false;


    if(WinHttpSendRequest(r,NULL,0,NULL,0,0,0))
    {
        if(WinHttpReceiveResponse(r,NULL))
        {
            DWORD size=0;
            int pos=0;

            while(
                WinHttpQueryDataAvailable(r,&size)
                &&
                size
            )
            {
                char buffer[4096];
                DWORD read=0;

                if(WinHttpReadData(
                    r,
                    buffer,
                    min(size,(DWORD)4095),
                    &read
                ))
                {
                    if(pos+read < sizeof(httpResult)-1)
                    {
                        memcpy(
                            httpResult+pos,
                            buffer,
                            read
                        );

                        pos+=read;
                        httpResult[pos]=0;
                    }

                    ok=true;
                }
            }
        }
    }


    WinHttpCloseHandle(r);
    WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);


    return ok;
}


char *json_value(const char *json,const char *key)
{
    static char value[512];

    value[0]=0;

    char search[128];

    formatstring(search)(
        "\"%s\"",
        key
    );


    char *p=strstr(
        (char*)json,
        search
    );


    if(!p)
        return value;


    p=strchr(
        p,
        ':'
    );


    if(!p)
        return value;


    p++;


    while(*p==' ' || *p=='\"')
        p++;


    int i=0;

    while(
        *p &&
        *p!='\"' &&
        *p!='\n' &&
        i<511
    )
    {
        value[i++]=*p++;
    }


    value[i]=0;

    return value;
}


void clearUpdateData()
{
    updateVersion[0]=0;
    updateTitle[0]=0;
    updateSha256[0]=0;
    updateFile[0]=0;
    updateTarget[0]=0;


    loopv(updateChanges)
        delete[] updateChanges[i];


    updateChanges.shrink(0);
}


void parseUpdateJSON()
{
    clearUpdateData();


    copystring(
        updateVersion,
        json_value(httpResult,"version"),
        sizeof(updateVersion)
    );


    copystring(
        updateTitle,
        json_value(httpResult,"title"),
        sizeof(updateTitle)
    );


    copystring(
        updateSha256,
        json_value(httpResult,"sha256"),
        sizeof(updateSha256)
    );


    copystring(
        updateFile,
        json_value(httpResult,"file"),
        sizeof(updateFile)
    );


    copystring(
        updateTarget,
        json_value(httpResult,"target"),
        sizeof(updateTarget)
    );
}


bool sha256_file(const char *file,char *out)
{
    HANDLE h=CreateFileA(
        file,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );


    if(h==INVALID_HANDLE_VALUE)
        return false;


    HCRYPTPROV prov;
    HCRYPTHASH hash;


    CryptAcquireContext(
        &prov,
        NULL,
        NULL,
        PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT
    );


    CryptCreateHash(
        prov,
        CALG_SHA_256,
        0,
        0,
        &hash
    );


    BYTE buffer[4096];
    DWORD read;


    while(
        ReadFile(
            h,
            buffer,
            sizeof(buffer),
            &read,
            NULL
        )
        &&
        read
    )
    {
        CryptHashData(
            hash,
            buffer,
            read,
            0
        );
    }


    BYTE result[32];
    DWORD len=32;


    CryptGetHashParam(
        hash,
        HP_HASHVAL,
        result,
        &len,
        0
    );


    for(int i=0;i<32;i++)
    {
        sprintf(
            out+(i*2),
            "%02x",
            result[i]
        );
    }


    out[64]=0;


    CryptDestroyHash(hash);
    CryptReleaseContext(prov,0);
    CloseHandle(h);


    return true;
}


void irx_get_http(char *url)
{
    if(http_get(url))
    {
        parseUpdateJSON();

        conoutf(
            "Update %s loaded",
            updateVersion
        );
    }
    else
    {
        conoutf("HTTP ERROR");
    }
}


void irx_fetch_url(char *url)
{
    irx_get_http(url);
}


void irx_download_url(char *url,char *file)
{
    if(http_get(url))
    {
        FILE *f=fopen(file,"wb");

        if(f)
        {
            fwrite(
                httpResult,
                1,
                strlen(httpResult),
                f
            );

            fclose(f);

            conoutf("Downloaded");
        }
    }
}


COMMAND(irx_get_http,"s");
COMMAND(irx_fetch_url,"s");
COMMAND(irx_download_url,"ss");