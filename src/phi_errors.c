#define PHI_RESULTS \
	RES(PHI_SUCCESS, 0) \
	RES(SCIF_OPEN_FAIL, 1)


#define RES(ENUM, VALUE) [VALUE] = #ENUM,
static const char *PHI_RESULT_STRING[] = {
	PHI_RESULTS
};
#undef RES

#define PHI_RESULT_STRING_ARR_SIZE \
	(int) (sizeof(PHI_RESULT_STRING)/sizeof(PHI_RESULT_STRING[0]))
