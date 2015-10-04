#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*******************************************
  Generate tones - multichannel version
  Based on https://github.com/lirongyuan/Music-Generator.git

  For WAVE PCM soundfile format see
  http://soundfile.sapp.org/doc/WaveFormat/
 ********************************************/

typedef struct WaveHeader {
	unsigned char chunkID[4];      // big endian 
	unsigned char chunkSize[4];    // little endian
	unsigned char format[4];       // big endian
	unsigned char subchunk1ID[4];  // big endian
	unsigned char subchunk1Size[4];// little endian
	unsigned char audioFormat[2]; // little endian
	unsigned char numChannels[2];  // little endian
	unsigned char sampleRate[4];   // little endian 
	unsigned char byteRate[4];     // little endian
	unsigned char blockAlign[2];   // little endian
	unsigned char bitsPerSample[2];// little endian
	unsigned char subchunk2ID[4];  // big endian
	unsigned char subchunk2Size[4];// little endian
	unsigned char data[1];         // little endian
} WaveHeader;

void assignLittleEndian4(unsigned char *dest, unsigned int value) {
	dest[0] = value & 0xFF;
	dest[1] = (value >> 8) & 0xFF;
	dest[2] = (value >> 16) & 0xFF;
	dest[3] = (value >> 24) & 0xFF;
}

/*
 * Updated version to write out samples in little-endian format no matter what this machine's architecture
 * Assumes 16-bit signed values currently
 */
void assignLittleEndian2signed(unsigned char *p, signed short value) {
	p[0] = (unsigned char)(value&0xff);
	p[1] = (unsigned char)((value>>8)&0xff);
}

int main(int argc, char ** argv) 
{
	if (argc <5) {
		printf("Usage: %s frequency(Hz) seconds channels outputFile\n", argv[0]);
		exit(1);
	}

	int freq;
	int secs;
	unsigned int numChannels;
	char outputFile[256];			// Allow for long filenames.  Note this is somewhat unsafe as not checkign for buffer overflow

	sscanf(argv[1], "%d", &freq);
	sscanf(argv[2], "%d", &secs);
	sscanf(argv[3], "%d", &numChannels);
	sscanf(argv[4], "%s", outputFile);

	printf("Generating: frequency %d Hz, duration %d seconds, %d identical channels, outputFile=%s\n", freq, secs, numChannels, outputFile);

	unsigned int sampleRate = 44100;
	unsigned int bitsPerSample = 16;
	unsigned int numSamples = secs * sampleRate;
	unsigned int dataSize = numSamples * numChannels * (bitsPerSample/8);
	unsigned int byteRate = numChannels * sampleRate * (bitsPerSample/8);

	
	// Subtract 1 for the data[1] in the header
	unsigned int fileSize = sizeof(WaveHeader) - 1 + dataSize;  

	printf("fileSize = %d\n", fileSize);

	// Allocate memory for header and data
	WaveHeader * hdr = (WaveHeader *) malloc(fileSize);		

	// Fill up header
	hdr->chunkID[0]='R'; hdr->chunkID[1]='I'; hdr->chunkID[2]='F'; hdr->chunkID[3]='F';
	assignLittleEndian4(hdr->chunkSize, fileSize - 8);
	hdr->format[0] = 'W'; hdr->format[1] = 'A'; hdr->format[2] = 'V'; hdr->format[3] = 'E';
	hdr->subchunk1ID[0]='f'; hdr->subchunk1ID[1]='m'; hdr->subchunk1ID[2]='t'; hdr->subchunk1ID[3]=' ';
	assignLittleEndian4( hdr->subchunk1Size,16);
	hdr->audioFormat[0] = 1; hdr->audioFormat[1]=0;
	hdr->numChannels[0] = numChannels; hdr->numChannels[1]=0;
	assignLittleEndian4(hdr->sampleRate, sampleRate);
	assignLittleEndian4(hdr->byteRate, byteRate);

	hdr->blockAlign[0] = numChannels * bitsPerSample/8; hdr->blockAlign[1]=0;
	hdr->bitsPerSample[0] = bitsPerSample; hdr->bitsPerSample[1]=0;
	hdr->subchunk2ID[0]='d'; hdr->subchunk2ID[1]='a'; hdr->subchunk2ID[2]='t'; hdr->subchunk2ID[3]='a';
	assignLittleEndian4(hdr->subchunk2Size, dataSize);

	// Generate data
	for (int i=0; i<numSamples; i++) {
		signed short value = 32767*sin(3.1415*freq*i/sampleRate);
		assignLittleEndian2signed(&hdr->data[i], value);		
	}

	// Write file to disk
	FILE *f = fopen(outputFile, "w+");
	if (f==NULL) {
		printf("Could not create file\n");
		perror("fopen");
		exit(1);
	}

	fwrite(hdr, fileSize, 1, f);
	printf("%d bytes written: %lu bytes header + %u bytes samples.\n", fileSize, sizeof(WaveHeader)-1, dataSize);

	fclose(f);
}