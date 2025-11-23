
#include "launcher.hpp"

int main(int argc, char** argv)
{
	Context ctx;
	if (Init(ctx, { argv, argv + argc })) {
		while (Update::Do(ctx, { GetFrameTime() }))
			Render::Do(ctx, { GetScreenWidth(), GetScreenHeight() });
		Release(ctx);
	}
	return 0;
}
