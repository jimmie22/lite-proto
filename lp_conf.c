#include <stdlib.h>
#include <stdio.h>

// ��ȡ�ļ��ֽڴ�С
long fsize( FILE *fp)
{
    long int save_pos;
    long size_of_file;
	
	if (fp == NULL)
		return 0;
	
    save_pos = ftell( fp );			// ���浱ǰ�ļ�ָ���ַ
	
    fseek( fp, 0L, SEEK_END );		// ��ת���ļ�ĩβ
    size_of_file = ftell( fp );		// ��ȡ�ļ���ʼĩβ�ļ���ַ
    fseek( fp, save_pos, SEEK_SET ); // �ָ���ǰ���ļ���ַ
	
    return( size_of_file);
}

void* lp_malloc(size_t len)
{
	return malloc(len);
}

void lp_free(void* p)
{
	free(p);
}

