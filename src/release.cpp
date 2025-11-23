
#include "launcher.hpp"

void ReleaseTcl(Context& ctx)
{
	Tcl_DeleteInterp(ctx.interp);
	ctx.interp = nullptr;
}

void ReleaseDB(Context& ctx)
{
	if (ctx.db) {
		sqlite3_close(ctx.db);
		ctx.db = nullptr;
	}
}

void Release(Context& ctx)
{
	ReleaseDB(ctx);
	CloseWindow();
}
