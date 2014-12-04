
#ifndef MIDGARD_TEST_USER_H
#define MIDGARD_TEST_USER_H

#include "midgard_test.h"

MidgardUser *midgard_test_user_auth(MidgardConnection *mgd, 
		const gchar *username, const gchar *password, const gchar *sitegroup);

void midgard_test_user_run(void);

#endif /* MIDGARD_TEST_USER_H */
