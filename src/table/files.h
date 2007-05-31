/* $Id$ */

/*
	MD5 sums of graphics files

	DOS -

	TRG1.GRF 9311676280e5b14077a8ee41c1b42192
	TRGC.GRF ed446637e034104c5559b32c18afe78d
	TRGH.GRF ee6616fb0e6ef6b24892c58c93d86fc9
	TRGI.GRF da6a6c9dcc451eec88d79211437b76a8
	TRGT.GRF fcde1d7e8a74197d72a62695884b909e
	SAMPLE.CAT 422ea3dd074d2859bb51639a6e0e85da

	WINDOWS -

	TRG1R.GRF b04ce593d8c5016e07473a743d7d3358
	TRGCR.GRF 3668f410c761a050b5e7095a2b14879b
	TRGHR.GRF 06bf2b7a31766f048baac2ebe43457b1
	TRGIR.GRF 0c2484ff6be49fc63a83be6ab5c38f32
	TRGTR.GRF de53650517fe661ceaa3138c6edb0eb8
	SAMPLE.CAT 9212e81e72badd4bbe1eaeae66458e10
*/


static FileList files_dos = {
	{
		{ "TRG1.GRF",      {0x93, 0x11, 0x67, 0x62, 0x80, 0xe5, 0xb1, 0x40, 0x77, 0xa8, 0xee, 0x41, 0xc1, 0xb4, 0x21, 0x92} }, //    0 - 4792 inclusive
		{ "TRGI.GRF",      {0xda, 0x6a, 0x6c, 0x9d, 0xcc, 0x45, 0x1e, 0xec, 0x88, 0xd7, 0x92, 0x11, 0x43, 0x7b, 0x76, 0xa8} }, // 4793 - 4889 inclusive
		{ "dosdummy.grf",  {0x07, 0x01, 0xe6, 0xc4, 0x07, 0x6a, 0x5b, 0xc3, 0xf4, 0x9f, 0x01, 0xad, 0x21, 0x6c, 0xa0, 0xc2} }, // 4890 - 4895 inclusive
		{ NULL, { 0 } }
	}, {
		{ "TRGC.GRF",      {0xed, 0x44, 0x66, 0x37, 0xe0, 0x34, 0x10, 0x4c, 0x55, 0x59, 0xb3, 0x2c, 0x18, 0xaf, 0xe7, 0x8d} },
		{ "TRGH.GRF",      {0xee, 0x66, 0x16, 0xfb, 0x0e, 0x6e, 0xf6, 0xb2, 0x48, 0x92, 0xc5, 0x8c, 0x93, 0xd8, 0x6f, 0xc9} },
		{ "TRGT.GRF",      {0xfc, 0xde, 0x1d, 0x7e, 0x8a, 0x74, 0x19, 0x7d, 0x72, 0xa6, 0x26, 0x95, 0x88, 0x4b, 0x90, 0x9e} }
	}
};

static FileList files_win = {
	{
		{ "TRG1R.GRF",     {0xb0, 0x4c, 0xe5, 0x93, 0xd8, 0xc5, 0x01, 0x6e, 0x07, 0x47, 0x3a, 0x74, 0x3d, 0x7d, 0x33, 0x58} }, //    0 - 4792 inclusive
		{ "TRGIR.GRF",     {0x0c, 0x24, 0x84, 0xff, 0x6b, 0xe4, 0x9f, 0xc6, 0x3a, 0x83, 0xbe, 0x6a, 0xb5, 0xc3, 0x8f, 0x32} }, // 4793 - 4895 inclusive
		{ NULL, { 0 } },
		{ NULL, { 0 } }
	}, {
		{ "TRGCR.GRF",     {0x36, 0x68, 0xf4, 0x10, 0xc7, 0x61, 0xa0, 0x50, 0xb5, 0xe7, 0x09, 0x5a, 0x2b, 0x14, 0x87, 0x9b} },
		{ "TRGHR.GRF",     {0x06, 0xbf, 0x2b, 0x7a, 0x31, 0x76, 0x6f, 0x04, 0x8b, 0xaa, 0xc2, 0xeb, 0xe4, 0x34, 0x57, 0xb1} },
		{ "TRGTR.GRF",     {0xde, 0x53, 0x65, 0x05, 0x17, 0xfe, 0x66, 0x1c, 0xea, 0xa3, 0x13, 0x8c, 0x6e, 0xdb, 0x0e, 0xb8} }
	}
};

static MD5File sample_cat_win = { "SAMPLE.CAT", {0x92, 0x12, 0xe8, 0x1e, 0x72, 0xba, 0xdd, 0x4b, 0xbe, 0x1e, 0xae, 0xae, 0x66, 0x45, 0x8e, 0x10} };
static MD5File sample_cat_dos = { "SAMPLE.CAT", {0x42, 0x2e, 0xa3, 0xdd, 0x07, 0x4d, 0x28, 0x59, 0xbb, 0x51, 0x63, 0x9a, 0x6e, 0x0e, 0x85, 0xda} };

static MD5File files_openttd[] = {
	{ "nsignalsw.grf", { 0x65, 0xb9, 0xd7, 0x30, 0x56, 0x06, 0xcc, 0x9e, 0x27, 0x57, 0xc8, 0xe4, 0x9b, 0xb3, 0x66, 0x81 } },
	{ "2ccmap.grf",    { 0x20, 0x03, 0x32, 0x1a, 0x43, 0x6c, 0xc1, 0x05, 0x80, 0xbd, 0x43, 0xeb, 0xe1, 0xfd, 0x0c, 0x62 } },
	{ "airports.grf",  { 0xfd, 0xa4, 0x38, 0xd6, 0x9c, 0x81, 0x74, 0xfe, 0xa0, 0x98, 0xa2, 0x14, 0x4b, 0x15, 0xb8, 0x4b } },
	{ "autorail.grf",  { 0xed, 0x44, 0x7f, 0xbb, 0x19, 0x44, 0x48, 0x4c, 0x07, 0x8a, 0xb1, 0xc1, 0x5c, 0x12, 0x3a, 0x60 } },
	{ "canalsw.grf",   { 0x13, 0x9c, 0x98, 0xcf, 0xb8, 0x7c, 0xd7, 0x1f, 0xca, 0x34, 0xa5, 0x6b, 0x65, 0x31, 0xec, 0x0f } },
	{ "elrailsw.grf",  { 0x4f, 0xf9, 0xac, 0x79, 0x50, 0x28, 0x9b, 0xe2, 0x15, 0x30, 0xa8, 0x1e, 0xd5, 0xfd, 0xe1, 0xda } },
	{ "openttd.grf",   { 0x85, 0x4f, 0xf6, 0xb5, 0xd2, 0xf7, 0xbc, 0x1e, 0xb9, 0xdc, 0x44, 0xef, 0x35, 0x5f, 0x64, 0x9b } },
	{ "trkfoundw.grf", { 0x12, 0x33, 0x3f, 0xa3, 0xd1, 0x86, 0x8b, 0x04, 0x53, 0x18, 0x9c, 0xee, 0xf9, 0x2d, 0xf5, 0x95 } },
	{ "roadstops.grf", { 0x8c, 0xd9, 0x45, 0x21, 0x28, 0x82, 0x96, 0x45, 0x33, 0x22, 0x7a, 0xb9, 0x0d, 0xf3, 0x67, 0x4a } },
	{ "group.grf",     { 0xe8, 0x52, 0x5f, 0x1c, 0x3e, 0xf9, 0x91, 0x9d, 0x0f, 0x70, 0x8c, 0x8a, 0x21, 0xa4, 0xc7, 0x02 } },
	{ "tramtrkw.grf",  { 0x83, 0x0a, 0xf4, 0x9f, 0x29, 0x10, 0x48, 0xfd, 0x76, 0xe9, 0xda, 0xac, 0x5d, 0xa2, 0x30, 0x45 } },
	{ "oneway.grf",    { 0xbb, 0xc6, 0xa3, 0xb2, 0xb3, 0xa0, 0xc9, 0x3c, 0xc9, 0xee, 0x24, 0x7c, 0xb6, 0x51, 0x74, 0x63 } },
};
