/*--------------------------------------------------------------------*/
/* SectorMap                                                          */
/* by GienekP                                                         */
/* (c) 2022                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
#define SECSIZE 256
/*--------------------------------------------------------------------*/
const unsigned char maxpos=((0xD500 & 0x1FFF)/SECSIZE);
const unsigned int maxsec=(((512*1024)/8192-1)*(8192-512))/256;
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	int i;
	unsigned char bank=1;
	unsigned char pos=0;
	FILE *pf;
	printf("SectorMap Generator\nMax Disk Size: %ibytes (%i sectors)\n",maxsec*SECSIZE,maxsec);
	pf=fopen("SectorMap.dta","wb");
	if (pf)
	{
		printf("Build SectorMap.dta\n");
		fputc(0xFF,pf);
		fputc(0xFF,pf);
		for (i=1; i<=maxsec; i++)
		{
			putc(bank,pf);
			putc(pos,pf);
			printf("Sector: %i -> Bank: %X  Pos: %02X ($%04X 0x%05X %i)\n",i,bank&0x3F,pos,SECSIZE*pos+0xA000,bank*8192+SECSIZE*pos,pos*256);
			pos++;
			if (pos==maxpos) {pos++;};
			if (pos==((8192-256)/256))
			{
				pos=0;
				bank++;
				bank&=0x3F;
			};
		};	
		fclose(pf);
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
