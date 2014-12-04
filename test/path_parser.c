#include "midgard_test.h"

static int my_hook(midgard *mgd, const char *table, const char *name, int up, int id, int flag) {
    if((flag == MIDGARD_PATH_ELEMENT_EXISTS)) {
	fprintf(stderr, "SnippetDir '%s' (ID=%d) already exists, its uplink is %d\n", name, id, up);
	return id;
    }
    if(flag == MIDGARD_PATH_ELEMENT_NOTEXISTS) {
        id = mgd_create(mgd, table, "name,up", "$q,$d", name,up);
	if(id) fprintf(stderr, "SnippetDir '%s' (ID=%d) added under SnippetDir with ID=%d\n", name, id, up);
	return id;
    }
    return 0;
}

void midgard_test_function(midgard *mgd) {
	int i,j,k;
	i = MGD_PARSE_ARTICLE_PATH_BY_TITLE(mgd, 
		"/The Example Site/News Releases/Site News/Example Site creation begun", 
		&k, &j);
        fprintf(stdout, "\n(%d) Requested topic\'s ID is %d, article = %d\n", i, j, k);
	
	i = MGD_PARSE_TOPIC_PATH(mgd, "/The Example Site/News Releases/Site News", &j);
        fprintf(stdout, "\n(%d) Requested topic\'s ID is %d\n", i, j);
	
	i = MGD_PARSE_COMMON_PATH(mgd, "/VMUC Root/about/site", "page", "page", &j, &k);
        fprintf(stdout, "\n(%d) Requested page's ID is %d, uplink is %d\n", i, j, k);
	
	i = mgd_parse_path_with_hook(mgd, "/Test01/Test02/Test03", "snippetdir", NULL, NULL, 
		NULL, NULL, &j, my_hook);
        fprintf(stdout, "\n(%d) SnippetDir Test03 created if not existed, ID = %d \n", i, j);
}
