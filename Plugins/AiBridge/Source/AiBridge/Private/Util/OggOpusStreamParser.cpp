// Fill out your copyright notice in the Description page of Project Settings.


#include "Util/OggOpusStreamParser.h"

void FOggOpusStreamParser::PushBytes(const uint8* Data, int32 Size)
{
	Buffer.Append(Data, Size);

	while (true)
	{
		TArray<uint8> Page;

		if (!TryExtractPage(Page))
			break;

		uint32 Serial = ParseSerial(Page);
		uint8 HeaderType = ParseHeaderType(Page);

		bool bBOS = (HeaderType & 0x02) != 0; //Beginning of stream (BOS)
		bool bEOS = (HeaderType & 0x04) != 0; //End of stream (EOS)

		if (bBOS)
		{
			ActiveStreams.Add(Serial);

			if (OnStreamStart.IsBound())
				OnStreamStart.Execute(Serial, Page);
		}

		if (OnPageReceived.IsBound())
			OnPageReceived.Execute(Serial, Page);

		if (bEOS)
		{
			ActiveStreams.Remove(Serial);

			if (OnStreamEnd.IsBound())
				OnStreamEnd.Execute(Serial);
		}
	}
}

int32 FOggOpusStreamParser::FindCapturePattern()
{
	for (int32 i = 0; i <= Buffer.Num() - 4; i++)
	{
		if (Buffer[i] == 'O' &&
			Buffer[i+1] == 'g' &&
			Buffer[i+2] == 'g' &&
			Buffer[i+3] == 'S')
		{
			return i;
		}
	}

	return INDEX_NONE;
}

bool FOggOpusStreamParser::TryExtractPage(TArray<uint8>& OutPage)
{
	int32 CaptureIndex = FindCapturePattern();

	if (CaptureIndex == INDEX_NONE)
		return false;

	if (CaptureIndex > 0)
		Buffer.RemoveAt(0, CaptureIndex);

	if (Buffer.Num() < 27)
		return false;

	uint8 PageSegments = Buffer[26];

	int32 HeaderSize = 27 + PageSegments;

	if (Buffer.Num() < HeaderSize)
		return false;

	int32 PayloadSize = 0;

	for (int32 i = 0; i < PageSegments; i++)
		PayloadSize += Buffer[27 + i];

	int32 PageSize = HeaderSize + PayloadSize;

	if (Buffer.Num() < PageSize)
		return false;

	OutPage.Append(Buffer.GetData(), PageSize);

	Buffer.RemoveAt(0, PageSize);

	return true;
}

uint32 FOggOpusStreamParser::ParseSerial(const TArray<uint8>& Page)
{
	return
		Page[14] |
		(Page[15] << 8) |
		(Page[16] << 16) |
		(Page[17] << 24);
}

uint8 FOggOpusStreamParser::ParseHeaderType(const TArray<uint8>& Page)
{
	return Page[5];
}