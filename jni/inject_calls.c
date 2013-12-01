#include <vim.h>
#include <sys/stat.h>
#include <android/log.h>
#define LOGD(tag,fmt) __android_log_print(ANDROID_LOG_DEBUG,(tag),(fmt))
#ifdef FEAT_INJ_CALLS
extern void out_c(unsigned c);

void
inject_call_start() {
	out_c(ESC);
	out_c('\t');
}

void
inject_data(char *data, int size) {
	while (size > 0) {
		out_c(*data);
		data++;
		size--;
	}
}

void
inject_str(char *str) {
	while (*str) {
		out_c(*str);
		str++;
	}
}

void
inject_call_end() {
	out_c(NUL);
	out_flush();
#ifdef FEAT_SYN_HL
	/* Clear all syntax states to force resyncing. */
	syn_stack_free_all(curwin->w_s);
#endif
	redraw_later(CLEAR);
}

int
clip_mch_own_selection(VimClipboard *cbd)
{
	/*
	 * Never actually own the clipboard.	If another application sets the
	 * clipboard, we don't want to think that we still own it.
	 */
	return FAIL;
}

void
clip_mch_lose_selection(VimClipboard *cbd)
{
	/* Nothing needs to be done here */
}

void
clip_mch_request_selection(VimClipboard *cbd)
{
	FILE *f = NULL;
	char *fn = "/sdcard/.clip";
	char_u	*buf = NULL;
	int		type = MCHAR;
	int l = 64;
	struct stat st;
  
	unlink(fn);
	inject_call_start();
	inject_str("getclip\t");
	inject_str(fn);
	inject_call_end();
	l = 100;
	while (l) {
		l--;
		if (!access(fn,F_OK)) break;
		usleep(10000);
	}
	if (l == 0) {
		LOGD("INJ","ERR1");
		return;
	}
	l = 1000;
	while (l) {
		l--;
	  stat(fn, &st);
		if (st.st_size >= 20) break;
		usleep(10000);
	}
	if (l == 0) {
		LOGD("INJ","ERR2");
		return;
	}
	f=fopen(fn,"r");
	fscanf(f,"%020d",&l);
	fclose(f);
	while (1) {
	  stat(fn, &st);
		if (st.st_size >= 20+l) break;
		usleep(10000);
	}
	buf = (char_u *)malloc(l+1);
	__android_log_print(ANDROID_LOG_DEBUG,"INJ size","%d",l);
	f=fopen(fn,"r");
	fseek(f,20,SEEK_SET);
	fread(buf,l,1,f);
	fclose(f);
	buf[l]='\0';
  LOGD("INJ buf=",buf);
	clip_yank_selection(type, buf, l, cbd);
	free(buf);
}

/*
 * Send the currently selected Vim text to the clipboard.
 */
	void
clip_mch_set_selection( VimClipboard *cbd )
{
	long_u	clip_data_size;
	int		clip_data_type;
	FILE *f;
	char_u  *p;
	char_u	*pMsg = NULL;
	char_u	*pClipData = NULL;

	/* If the '*' register isn't already filled in, fill it in now. */
	cbd->owned = TRUE;
	clip_get_selection(cbd);
	cbd->owned = FALSE;

	/* clip_convert_selection() returns a pointer to a buffer containing
	 * the text to send to the clipboard, together with a count
	 * of the number of characters (bytes) in the buffer.	The function's
	 * return value is the 'type' of selection: MLINE, MCHAR, or MBLOCK;
	 * or -1 for failure.
	 */
	clip_data_type = clip_convert_selection(&pClipData, &clip_data_size, cbd);

	if (clip_data_type < 0)  /* could not convert? */
			return; /* early exit */

	for (p=pClipData; p<pClipData+clip_data_size; p++) {
		if (*p==NUL) *p='\n';
		inject_call_start();
		inject_str("setclip\t");
		inject_data(pClipData,clip_data_size);
		inject_call_end();
	}
}
#endif
// vim: set sw=2 ts=2 sts=2 :
