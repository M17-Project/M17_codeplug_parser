#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_CHANNELS_PER_BANK			20			//max channels allowed in a bank
#define MAX_NAME_LEN					12+1		//max text length (except for VERSION -> 20)
#define MAX_BANK_NUM					100			//bank capacity in channels

#define K_AUTHOR						"author"
#define AUTHOR							1
#define K_VERSION						"version"
#define VERSION							2

#define K_NUM_BANKS						"num_banks"
#define NUM_BANKS						3
#define K_NUM							"num"
#define NUM								4
#define K_NAME							"name"
#define NAME							5
#define K_NUM_CHANNELS					"num_channels"
#define NUM_CHANNELS					6
#define K_DESCR							"descr"
#define DESCR							7
#define K_FREQ							"freq"
#define FREQ							8
#define K_ENCR							"encr"
#define ENCR							9
#define K_GPS							"gps"
#define GPS								10

#define EMPTY_OR_COMMENT				255

#define K_CODEPLUG						"codeplug"
#define CODEPLUG						100
#define K_BANK							"bank"
#define BANK							101
#define K_CHANNEL						"channel"
#define CHANNEL							102
#define K_END							"end"
#define END								200

uint8_t in_codeplug=0, in_bank=0, in_channel=0;		//where are we?
uint8_t bank_num=0, channel_num=0;					//

struct CHANNEL_DATA {
	volatile uint8_t		name[MAX_NAME_LEN];		//channel name
	volatile uint8_t		descr[MAX_NAME_LEN];	//channel description
	volatile uint32_t		freq;					//frequency in Hz
	volatile uint8_t		enc_type;				//encryption type
	volatile uint8_t		gps;					//gps data transmission type
	volatile uint16_t		tg_id;					//talkgroup ID
};

struct BANK_DATA {
	volatile uint8_t name[MAX_NAME_LEN];
	volatile uint8_t num_channels;
	struct CHANNEL_DATA channel[MAX_CHANNELS_PER_BANK];
};

struct CODEPLUG_DATA {
	volatile uint8_t		author[MAX_NAME_LEN];
	volatile uint8_t		version[20];
	volatile uint8_t		name[MAX_NAME_LEN];
	volatile uint16_t		num_banks;
	struct BANK_DATA		bank[MAX_BANK_NUM];
};

struct CODEPLUG_DATA codeplug;

FILE *cp_fil;

uint8_t file_line[500][100];
uint32_t lines=0;

uint8_t test[256];

uint32_t extractValue(uint8_t *line)
{	
	for(uint8_t i=0; i<100; i++)
	{
		if(line[i]=='=')
			return atoi(&line[i+1]);
	}
	
	return 0;
}

void extractString(uint8_t *line, uint8_t *dest)
{	
	for(uint8_t i=0; i<100; i++)
	{
		if(line[i]=='"')
		{
			uint8_t cnt=0;
			
			while(line[i+1+cnt]!='"')
			{
				dest[cnt]=line[i+1+cnt];
				cnt++;
			}
			break;
		}
	}
}

uint8_t extractType(uint8_t *line)
{
	//comment
	if(line[0]=='#')
		return EMPTY_OR_COMMENT;
	else if(strstr(line, K_CODEPLUG) != NULL)
		return CODEPLUG;
	else if(strstr(line, K_AUTHOR) != NULL)
		return AUTHOR;
	else if(strstr(line, K_VERSION) != NULL)
		return VERSION;
	else if(strstr(line, K_NUM_BANKS) != NULL)
		return NUM_BANKS;
	else if(strstr(line, K_NUM_CHANNELS) != NULL)
		return NUM_CHANNELS;
	else if(strstr(line, K_NUM) != NULL)
		return NUM;
	else if(strstr(line, K_FREQ) != NULL)
		return FREQ;
	else if(strstr(line, K_ENCR) != NULL)
		return ENCR;
	else if(strstr(line, K_GPS) != NULL)
		return GPS;
	else if(strstr(line, K_DESCR) != NULL)
		return DESCR;
	else if(strstr(line, K_NAME) != NULL)
		return NAME;
		
	else if(strstr(line, K_BANK) != NULL)
		return BANK;
	else if(strstr(line, K_CHANNEL) != NULL)
		return CHANNEL;
	else if(strstr(line, K_END) != NULL)
		return END;
		
	return EMPTY_OR_COMMENT;
}

void parseCodeplug(struct CODEPLUG_DATA *cp)
{
	for(uint8_t line=0; line<lines; line++)
	{
		if(extractType(file_line[line])==EMPTY_OR_COMMENT)
			continue;
			
		switch(extractType(file_line[line]))
		{
			case CODEPLUG:
				in_codeplug=1;
			break;
			
			case BANK:
				in_bank=1;
				bank_num++;
			break;
			
			case END:
				if(in_channel)
					{in_channel=0; break;}
				if(in_bank)
					{in_bank=0; break;}
				if(in_codeplug)
					in_codeplug=0;
			break;
			
			case AUTHOR:
				extractString(file_line[line], cp->author);
			break;
			
			case VERSION:
				extractString(file_line[line], cp->version);
			break;
			
			case NUM_BANKS:
				cp->num_banks=extractValue(file_line[line]);
			break;
			
			case NUM_CHANNELS:
				cp->bank[bank_num-1].num_channels=extractValue(file_line[line]);
			break;
			
			case CHANNEL:
				in_channel=1;
				channel_num++;
			break;
			
			case NAME:
				if(in_bank && !in_channel)
					extractString(file_line[line], cp->bank[bank_num-1].name);
				else if(in_bank && in_channel)
					extractString(file_line[line], cp->bank[bank_num-1].channel[channel_num-1].name);
			break;
			
			case DESCR:
				if(in_channel)
					extractString(file_line[line], cp->bank[bank_num-1].channel[channel_num-1].descr);
			break;
			
			case FREQ:
				if(in_channel)
					cp->bank[bank_num-1].channel[channel_num-1].freq=extractValue(file_line[line]);
			break;
			
			case ENCR:
				if(in_channel)
					cp->bank[bank_num-1].channel[channel_num-1].enc_type=extractValue(file_line[line]);
			break;
			
			case GPS:
				if(in_channel)
					cp->bank[bank_num-1].channel[channel_num-1].gps=extractValue(file_line[line]);
			break;
			
			default:
				;
			break;
		}
	}
}

int main(void)
{
	cp_fil = fopen("2600653.m17", "rb");
	
	while(fgets(&file_line[lines++], 100, cp_fil)!=NULL);
	
	printf("Loaded %d lines\n", lines);
	
	parseCodeplug(&codeplug);
	
	printf("Author: %s\n", codeplug.author);
	printf("Version: %s\n", codeplug.version);
	printf("Banks: %d\n\n", codeplug.num_banks);
	
	for(uint8_t i=0; i<codeplug.num_banks; i++)
	{
		printf("Bank %d channels:\t%d\n", i, codeplug.bank[i].num_channels);
		printf("Bank %d name:\t\t%s\n\n", i, codeplug.bank[i].name);
		
		for(uint8_t j=0; j<codeplug.bank[i].num_channels; j++)
		{
			printf("Bank %d Channel %d name:\t%s\n", i, j, codeplug.bank[i].channel[j].name);
			printf("Bank %d Channel %d descr:\t%s\n", i, j, codeplug.bank[i].channel[j].descr);
			printf("Bank %d Channel %d freq:\t%d\n", i, j, codeplug.bank[i].channel[j].freq);
			printf("Bank %d Channel %d encr:\t%d\n", i, j, codeplug.bank[i].channel[j].enc_type);
			printf("Bank %d Channel %d gps:\t%d\n", i, j, codeplug.bank[i].channel[j].gps);
			printf("\n");
		}
	}
	
	fclose(cp_fil);
	
	return 0;
}

