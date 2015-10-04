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

void assignLittleEndian4(unsigned char *p, unsigned int value) {
	*p++ = value & 0xFF;
	value >>= 8;
	*p++ = value & 0xFF;
	value >>= 8;
	*p++ = value & 0xFF;
	value >>= 8;
	*p++ = value & 0xFF;
}

void assignLittleEndian4str(unsigned char *p, char *s) {
	*p++ = *s++;		// Can just copy simple 8-bit chars without realignment
	*p++ = *s++;
	*p++ = *s++;
	*p++ = *s++;
}

/*
 * Assumes 16-bit signed values for samples currently - would need extensions to work with larger sample sizes
 */
void assignLittleEndian2signed(unsigned char *p, signed short value) {
	*p++ = value & 0xFF;
	value >>= 8;
	*p++ = value & 0xFF;
}

void assignLittleEndian2(unsigned char *p, unsigned short value) {
	*p++ = value & 0xFF;
	value >>= 8;
	*p++ = value & 0xFF;
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
	unsigned int bytesPerSample = bitsPerSample/8;
	unsigned int numSamples = secs * sampleRate;
	unsigned int dataSize = numSamples * numChannels * bytesPerSample;
	unsigned int byteRate = numChannels * sampleRate * bytesPerSample;
	
	// Subtract 1 for the data[1] in the header
	unsigned int fileSize = sizeof(WaveHeader) - 1 + dataSize;  

	// Allocate memory for header and data
	WaveHeader * hdr = (WaveHeader *) malloc(fileSize);		

	// Start file with RIFF header

	assignLittleEndian4str(hdr->chunkID, "RIFF");
	assignLittleEndian4(hdr->chunkSize, fileSize - 8);			// chunkID and chunkSize longwords don't count towards the chunkSize itself

	// Subchunk 1 - WAVE header

	assignLittleEndian4str(hdr->format, "WAVE");
	assignLittleEndian4str(hdr->subchunk1ID, "fmt ");
	assignLittleEndian4(hdr->subchunk1Size,16);					// Header is always 16 bytes
	assignLittleEndian2(hdr->audioFormat, 1);					// PCM = 1
	assignLittleEndian2(hdr->numChannels, numChannels);
	assignLittleEndian4(hdr->sampleRate, sampleRate);
	assignLittleEndian4(hdr->byteRate, byteRate);
	assignLittleEndian2(hdr->blockAlign, numChannels * bytesPerSample);
	assignLittleEndian2(hdr->bitsPerSample, bitsPerSample);

	// Subchunk 2 - sample data

	assignLittleEndian4str(hdr->subchunk2ID, "data");
	assignLittleEndian4(hdr->subchunk2Size, dataSize);

	unsigned char *p = &hdr->data[0];
	for (int i=0; i<numSamples; i++) {
		signed short value = 32767*sin(3.1415*freq*i/sampleRate);
		assignLittleEndian2signed(p, value);	
		p+=2;	
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