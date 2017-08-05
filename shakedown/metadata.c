#include "metadata.h"
#include "box.h"

void print_build_configuration()
{
	box_top();
	box_print("Project name: %s", BUILD_NAME_STR);
	box_print("Project version: v%s", BUILD_VERSION_STR);
	box_horiz();
	box_print("Git branch: %s", GIT_BRANCH_STR);
	box_print("Git commit: %s (%s)", GIT_COMMIT_STR, GIT_DIRTY_STR);
	box_horiz();
	box_print("Build UUID: %s", BUILD_UUID_STR);
	box_print("Build ID: %s", BUILD_ID_STR);
	box_print("Build description: %s", BUILD_DESC_STR);
	box_print("Build date: %s", BUILD_DATE_STR);
	box_print("Build by: %s <%s>", BUILD_USER_STR, BUILD_EMAIL_STR);
	box_horiz();
	box_print("GCC version: %s", GCC_VERSION_STR);
	box_print("GCC optimisation mode: %s", GCC_OPTIMISE_STR);
	box_bottom();
}
