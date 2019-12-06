#include "iostream"
#include "stdio.h"
#include "iomanip"
#include "stdlib.h"
#include "cstring"
#include "windows.h"
using namespace std;

#define MAXSIZE 256

int read_lnk(const char* lnk_name, char* link_path, unsigned link_path_len)
{
    //���ļ���ȡ����
    FILE* pFile = NULL;
    pFile = fopen(lnk_name, "rb");
    if(!pFile)
    {
        return -1;
    }
    long iLow;
    long iHigh;
    long iPathLen;
    int iHaveSil = 0;
    //ǰ����14h��, �ж��Ƿ���shell item list
    fseek(pFile, 20L, SEEK_CUR);
    iLow = fgetc(pFile);
    if(iLow & 1 == 1)
    {
        iHaveSil = 1;
    }
    //ǰ����4ch��, �ҵ�sil
    fseek(pFile, 76L, SEEK_SET);
    iLow = fgetc(pFile);
    iHigh = fgetc(pFile);
    //ǰ�����ļ�λ����Ϣ�ĵط�
    fseek(pFile, iLow, SEEK_CUR);
    int i = 0;
    for(i = 0; i < iHigh; i += 1)
    {
        fseek(pFile, 256L, SEEK_CUR);
    }
    //�õ������ļ���Ϣ��ƫ��
    fseek(pFile, 16L, SEEK_CUR);
    iLow = fgetc(pFile);
    //�õ�������Ϣ��ƫ��
    fseek(pFile, 7L, SEEK_CUR);
    iHigh = fgetc(pFile);
    //�õ�·���ĳ���
    iPathLen = iHigh - iLow;
    if (iPathLen >= link_path_len)
        return -1;
    //�н�������·����Ϣ
    fseek(pFile, -25L, SEEK_CUR);
    fseek(pFile, iLow, SEEK_CUR);
    //���·����Ϣ
    for(i = 0; i < iPathLen; i += 1)
    {
        link_path[i] = fgetc(pFile);
        //���ȡ��, ��ֹͣ
        if(link_path[i] == 0)
        {
            link_path[i] = '\0';
            break;
        }
    }
    //���˳�����·����Ϣ
    for(i = strlen(link_path)-1; i>=0; i -= 1)
    {
        if(link_path[i] == '\\')
            break;
        else link_path[i] = '\0';
    }
    fclose(pFile);
    return 0;
}