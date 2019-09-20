#include <stdlib.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

int get_callinfo(char *fname, size_t fnlen, unsigned long long *ofs)
{
	unw_context_t ctxt;
	unw_cursor_t cp;
	unw_word_t offp;
	unsigned long long cnt = 0;
	
	if(unw_getcontext(&ctxt)!=0){
		return -1;
	}
	if(unw_init_local(&cp, &ctxt)!=0){
		return -1;
	}
	for(int i=0;i<3;i++){
		unw_step(&cp);
		if(unw_get_proc_name(&cp, fname, fnlen, &offp)!=0){
			break;
		}
	}
	*ofs = (unsigned long long int)(offp-5);
	
	return 0;
}
