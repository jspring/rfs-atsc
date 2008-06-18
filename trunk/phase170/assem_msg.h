// Unions for assembling and extracting a packet // 
union pack_short
{
	short s;
	unsigned char c[2];
};

union pack_interger
{
	int i;
	unsigned char c[4];
};

union pack_float
{
	float f;
	unsigned char c[4];
};

int InsertChar(unsigned char *packet_array, int npack, char value)
{
	int i = npack;
	packet_array[i++] = value;
	return(i);
}

int InsertShortValue(unsigned char *packet_array, int npack, int value)
{
	union pack_short ps;
	int i = npack;
	ps.s = (short)value;
	packet_array[i++] = ps.c[0];
	packet_array[i++] = ps.c[1];
	return(i);
}

int InsertIntergerValue(unsigned char *packet_array, int npack, int value)
{
	union pack_interger pi;
	int i = npack,j;
	pi.i = value;
	for (j=0;j<4;j++) packet_array[i++] = pi.c[j];
	return(i);
}

int InsertFloatValue(unsigned char *packet_array, int npack, float value)
{
	union pack_float pf;
	int i = npack,j;
	pf.f = value;
	for (j=0;j<4;j++) packet_array[i++] = pf.c[j];
	return(i);
}
