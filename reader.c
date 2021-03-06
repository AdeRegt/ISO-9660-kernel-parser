#include <stdio.h>
#include <stdlib.h>

#define CDROMSECTORSIZE 2048
#define VERSION 1

FILE *bestand;
unsigned char buffer[CDROMSECTORSIZE];

void helpMessage(){
	printf("reader option cdromimage.iso\n");
	printf("--help             see this message\n");
	printf("--version          get programversion\n");
	printf("--list    ISO      list contents of ISO file\n");
	printf("--display ISO PATH display contents of file\n");
	printf("--tree    ISO      prints tree of ISO file\n");
}

void seek(long count){
	long size = count*CDROMSECTORSIZE;
	int i = 0;
	fseek(bestand,size,SEEK_SET);
	for(i = 0 ; i < CDROMSECTORSIZE ; i++){
		buffer[i] = (fgetc(bestand) & 0x00ff);
	}
}

unsigned char getB(int a){
	return buffer[a] & 0xff;
}

unsigned short getS(short a){
	return (short)((short*)&buffer[a])[0];
}


unsigned long getL(long a){
	return (long)((long*)&buffer[a])[0];
}

unsigned long getPrimairyVolumeDescriptor(){
	int i = 0 ; 
	for(i = 0 ; i < 100 ; i++){
		seek(0x10+i);
		if(getB(1)=='C'&&getB(2)=='D'&&getB(3)=='0'&&getB(4)=='0'&&getB(5)=='1')
			if(getB(0)==1){
				return 0x10+i;
			}
	}
	return 1;
}

void main(int argscount,char** arguments){
	if(argscount==2){
		printf("Argscount is oké: opening '%s'\n",arguments[1]);
		if(strcmp(arguments[1],"--help")==0){
			helpMessage();
			return;
		}else if(strcmp(arguments[1],"--version")==0){
			printf("Version: %i\n",VERSION);
			return;
		}else{
			helpMessage();
			return;
		}
	}else if(argscount==3){
		bestand = fopen(arguments[2],"rb");
		if(bestand){
			seek(0);
			printf("BOOTABLE: %s\n",getB(510)==0x55&&getB(511)==0xAA?"YES":"NO");
			unsigned long primairyVolume = getPrimairyVolumeDescriptor();
			printf("PVD     : %i\n",(unsigned int)primairyVolume);
			unsigned short sectorsize = getS(128);
			if(sectorsize!=CDROMSECTORSIZE){
				printf("ERROR: invalid sectorsize, expected: %i found: %i\n",CDROMSECTORSIZE,sectorsize);
				goto end;
			}
			unsigned short ltableloc = getS(140);
			printf("DirTable: %i\n",ltableloc);
			seek(ltableloc);
			unsigned int x = 0 ;
			unsigned int i = 0;
			while(1){
				char namesize = getB(i);
				if(namesize==0){break;}
				char attrlen = getB(i+1);
				int extlba = getL(i+2);
				short par = getS(i+6);
				int u;
				for(u = 0 ; u < namesize ; u++){
					printf("%c",getB(i+8+u));
				}
				unsigned int skippy = 8+namesize+attrlen;
				if(skippy % 2){
					skippy = skippy + 1;
				}
				i += skippy;
				printf("\t :: NMESZE=%i ATTR=%i LBA=%i PARN=%i \n",namesize,attrlen,extlba,par);
				seek(extlba);
				int g = 0 ;
				int d = 0;
				for( g = 0 ; g < 10 ; g++){
					unsigned char lengthofrecord = getB(d);
					unsigned char extended = getB(d+1);
					if(lengthofrecord==0){
						break;
					}
					unsigned int lbaloc = getL(d+2);
					unsigned int size = getL(d+10);
					unsigned char textsize = getB(d+32);
					unsigned char flags = getB(d+25);
					printf("\t => ");
					unsigned char q = 0;
					for(q = 0 ; q < textsize ; q++){
						printf("%c",getB(d+33+q));
					}
					printf("\t%s :: %s INTOFREC=%i EXT=%i LBA=%x SIZE=%x TXTSZE=%i \n",(textsize>10)?"":"\t",(flags & 2)?"DIR":"FIL",lengthofrecord,extended,lbaloc,size,textsize);
					d += lengthofrecord;
				}
				seek(ltableloc);
			}
			end:
			fclose(bestand);
		}else{
			printf("ERROR: unable to open %s\n",arguments[2]);
		}
	}else{
		helpMessage();
	}
}

