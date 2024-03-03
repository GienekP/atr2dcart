/*--------------------------------------------------------------------*/
/* ATR2DCART                                                          */
/* by GienekP                                                         */
/* (c) 2022                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
typedef unsigned char U8;
/*--------------------------------------------------------------------*/
#define START (0xD500)
/*--------------------------------------------------------------------*/
#define SEC128 0
#define SEC256_1 1
#define SEC256_2 2
#define OLDADDR 0
#define NEWADDR 1
/*--------------------------------------------------------------------*/
#define CARMAX (512*1024)
#define BANKSIZE (8*1024)
#define PROCSIZE (512)
#define ATRMAX (((CARMAX/BANKSIZE)-1)*(BANKSIZE-PROCSIZE))
#define D500 (0x1500)
#define BF00 (0x1F00)
#define BANKNUM (0x1FF9)
/*--------------------------------------------------------------------*/
#include "starter.h"
/*--------------------------------------------------------------------*/
U8 checkATR(const U8 *data)
{
	U8 ret=0;
	unsigned int i;
	if ((data[0]==0x96) && (data[1]==02))
	{
		if ((data[4]==0x80) && (data[5]==0x00))
		{
			ret=SEC128;
		}
		else if ((data[4]==0x00) && (data[5]==0x01))
		{
			if (data[2]&0x0F)
			{
				ret=SEC256_2;
			}
			else
			{
				ret=SEC256_1;
			};
			for (i=0; i<(BANKSIZE-4); i++)
			{
				if ( (starter_bin[i]==0x18) && (starter_bin[i+1]==0xFF) 
				  && (starter_bin[i+2]==0x01) && (starter_bin[i+3]==0x00) )
				{
					starter_bin[i]=0x38;
					i=BANKSIZE;
				};
			};
		};
	};
	return ret;
}
/*--------------------------------------------------------------------*/
unsigned int loadATR(const char *filename, unsigned int atrsize, U8 *data)
{
	U8 header[16];
	unsigned int i,ret=0;
	int j;
	FILE *pf;
	for (i=0; i<atrsize; i++) {data[i]=0xFF;};
	pf=fopen(filename,"rb");
	if (pf)
	{
		i=fread(header,sizeof(U8),16,pf);
		if (i==16)
		{
			U8 type=checkATR(header);
			switch (type)
			{
				case SEC128:
				{
					printf("Sector 128\n");
					ret=fread(data,sizeof(U8),atrsize/2,pf);
					
					for (j=((ret>>7)-1); j>=0; j--)
					{
						for (i=0; i<128; i++)
						{
							data[j*256+i]=data[j*128+i];
							data[j*256+128+i]=0xFF;
						};
					};
					ret<<=1;
				} break;
				case SEC256_1:
				{
					printf("Sector 256 type 1\n");
					ret+=fread(data,sizeof(U8),128,pf);
					ret+=128;
					ret+=fread(&data[ret],sizeof(U8),128,pf);
					ret+=128;
					ret+=fread(&data[ret],sizeof(U8),128,pf);
					ret+=128;
					fread(&data[ret],sizeof(U8),3*128,pf);
					ret+=fread(&data[ret],sizeof(U8),atrsize-768,pf);	
				} break;
				case SEC256_2:
				{
					printf("Sector 256 type 2\n");
					ret+=fread(data,sizeof(U8),128,pf);
					ret+=128;
					ret+=fread(&data[ret],sizeof(U8),128,pf);
					ret+=128;
					ret+=fread(&data[ret],sizeof(U8),128,pf);
					ret+=128;
					ret+=fread(&data[ret],sizeof(U8),atrsize-768,pf);					
				} break;
				default:
				{
					printf("Unknown sector header.\n");
				} break;				
			};
		}
		else
		{
			printf("Wrong ATR header size.\n");
		}
		fclose(pf);
	}
	else
	{
		printf("\"%s\" does not exist.\n",filename);
	};
	return ret;
}
/*--------------------------------------------------------------------*/
void replace(U8 mode, U8 *atrdata, unsigned int i, unsigned int ramproc)
{
	static U8 f=0;
	if (mode==NEWADDR)
	{
		atrdata[i+1]=((ramproc)&0xFF);
		atrdata[i+2]=(((ramproc)>>8)&0xFF);
	};
	if (f==0)
	{
		f=1;
		if (mode==NEWADDR)
		{
			printf("Replace calls:\n");
		}
		else
		{
			printf("Possible calls:\n");
		};
	};
}
/*--------------------------------------------------------------------*/
void checkXINT(U8 *atrdata, U8 mode)
{
	unsigned int i;
	unsigned int FSIOINT=START;
	unsigned int FDSKINT=START;
	for (i=0; i<(BANKSIZE-4); i++)
	{
		if ( (starter_bin[i]=='F') && (starter_bin[i+1]=='D') 
				  && (starter_bin[i+2]=='1') && (starter_bin[i+3]=='I') )
		{
			FDSKINT+=((0xA000+(i+4))-0xB500);
			i=BANKSIZE;
		};
	};
	printf("0x%04X\n",FDSKINT);
	for (i=0; i<ATRMAX-3; i++)
	{
		if ((atrdata[i+1]==0x53) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i,FDSKINT);
				printf(" JSR JDSKINT ; 0x%06X 20 53 E4 -> 20 %02X %02X\n",i+16,FDSKINT%256,FDSKINT/256);
				
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i,FDSKINT);
				printf(" JMP JDSKINT ; 0x%06X 4C 53 E4 -> 4C %02X %02X\n",i+16,FDSKINT%256,FDSKINT/256);
			};			
		};
		if ((atrdata[i+1]==0xB3) && (atrdata[i+2]==0xC6))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i,FDSKINT);
				printf(" JSR DSKINT ; 0x%06X 20 B3 C6 -> 20 %02X %02X\n",i+16,FDSKINT%256,FDSKINT/256);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i,FDSKINT);
				printf(" JMP DSKINT ; 0x%06X 4C B3 C6 -> 4C %02X %02X\n",i+16,FDSKINT%256,FDSKINT/256);
			};			
		};
		if ((atrdata[i+1]==0x59) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i,FSIOINT);
				printf(" JSR JSIOINT ; 0x%06X 20 59 E4 -> 20 %02X %02X\n",i+16,FSIOINT%256,FSIOINT/256);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i,FSIOINT);
				printf(" JMP JSIOINT ; 0x%06X 4C 59 E4 -> 4C %02X %02X\n",i+16,FSIOINT%256,FSIOINT/256);
			};			
		};
		if ((atrdata[i+1]==0x33) && (atrdata[i+2]==0xC9))
		{
			if (atrdata[i]==0x20)
			{
				replace(mode,atrdata,i,FSIOINT);
				printf(" JSR SIOINT ; 0x%06X 20 33 C9 -> 20 %02X %02X\n",i+16,FSIOINT%256,FSIOINT/256);
			};
			if (atrdata[i]==0x4C)
			{
				replace(mode,atrdata,i,FSIOINT);
				printf(" JMP SIOINT ; 0x%06X 4C 33 C9 -> 4C %02X %02X\n",i+16,FSIOINT%256,FSIOINT/256);
			};			
		};	
	};
}
/*--------------------------------------------------------------------*/
void buildCar(const U8 *loader, unsigned int loadersize, 
              const U8 *atrdata, unsigned int atrsize, 
              U8 *cardata, unsigned int carsize)
{
	unsigned int i,j;
	for (i=0; i<CARMAX; i++) {cardata[i]=0xFF;};
	for (i=0; i<loadersize; i++) {cardata[i]=loader[i];};
	for (i=1; i<(CARMAX/BANKSIZE); i++)
	{
		for (j=0; j<256; j++)
		{
			cardata[i*BANKSIZE+BF00+j]=cardata[BF00+j];
			cardata[i*BANKSIZE+D500+j]=cardata[D500+j];
		};
		cardata[i*BANKSIZE+BANKNUM]=i;
	};
	for (i=0; i<atrsize; i++)
	{
		unsigned int sec=(i>>8)+1;
		unsigned int adr,bank,pos;
		bank=loader[2*sec];
		pos=loader[2*sec+1];
		bank*=BANKSIZE;
		pos*=256;
		adr=(bank+pos)+(i%256);
		cardata[adr]=atrdata[i];
	};
}
/*--------------------------------------------------------------------*/
U8 saveCAR(const char *filename, U8 *cardata, unsigned int carsize)
{
	U8 header[16]={0x43, 0x41, 0x52, 0x54, 0x00, 0x00, 0x00, 0x70,
		           0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
	U8 ret=0;
	unsigned int i,sum=0;
	FILE *pf;
	for (i=0; i<carsize; i++) {sum+=cardata[i];};
	header[8]=((sum>>24)&0xFF);
	header[9]=((sum>>16)&0xFF);
	header[10]=((sum>>8)&0xFF);
	header[11]=(sum&0xFF);
	pf=fopen(filename,"wb");
	if (pf)
	{
		i=fwrite(header,sizeof(U8),16,pf);
		if (i==16)
		{
			i=fwrite(cardata,sizeof(U8),carsize,pf);
			if (i==carsize) {ret=1;};			
		};
		fclose(pf);
	};
	return ret;
}
/*--------------------------------------------------------------------*/
void atr2dcart(const char *atrfn, const char *carfn, U8 mode)
{
	U8 atrdata[ATRMAX];
	U8 cardata[CARMAX];
	unsigned int atrsize;
	atrsize=loadATR(atrfn,sizeof(atrdata),atrdata);
	if (atrsize)
	{
		printf("Load \"%s\"\n",atrfn);
		printf("ATR sectors: %i\n",(atrsize>>8));
		checkXINT(atrdata,mode);
		buildCar(starter_bin,starter_bin_len,atrdata,atrsize,cardata,sizeof(cardata));
		if (saveCAR(carfn,cardata,sizeof(cardata))) {printf("Save \"%s\"\n",carfn);}
		else {printf("Save \"%s\" ERROR!\n",carfn);};
	}
	else
	{
		printf("Load \"%s\" ERROR!\n",atrfn);
	};
}
/*--------------------------------------------------------------------*/
void modeSel(const char *str, U8 *mode)
{
	if ((str[0]=='-') && ((str[1]=='c') || (str[1]=='C')) && (str[2]==0))
	{
		*mode=NEWADDR;
	};
}
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	U8 mode=OLDADDR;
	printf("ATR2DCART - ver: %s\n",__DATE__);
	if (argc==3)
	{
		atr2dcart(argv[1],argv[2],mode);
	}
	else if (argc==4)
	{
		modeSel(argv[3],&mode);
		atr2dcart(argv[1],argv[2],mode);
	}
	else
	{
		printf("(c) GienekP\n");
		printf("use:\natr2dcart file.atr file.car [-c]\n");
		printf("-c  remap JSR & JMP with JDSKINT / DSKINT\n");
	};
	printf("\n");
	return 0;
}
/*--------------------------------------------------------------------*/
