
#include <stdlib.h>

#include <mlibc/ensure.h>
#include <mlibc/cxx-support.hpp>
#include <mlibc/posix-pipe.hpp>

void __mlibc_initMalloc();

// declared in posix-pipe.hpp
frigg::LazyInitializer<helx::EventHub> eventHub;
frigg::LazyInitializer<helx::Pipe> posixPipe;

// declared in posix-pipe.hpp
int64_t allocPosixRequest() {
	static int64_t next = 1;
	return next++;
}

struct LibraryGuard {
	LibraryGuard();
};

static LibraryGuard guard;

LibraryGuard::LibraryGuard() {
	// FIXME: initialize malloc here
	//__mlibc_initMalloc();

	eventHub.initialize(helx::EventHub::create());
	
	const char *posix_path = "local/posix";
	HelHandle posix_handle;
	HEL_CHECK(helRdOpen(posix_path, strlen(posix_path), &posix_handle));
	
	int64_t async_id;
	HEL_CHECK(helSubmitConnect(posix_handle, eventHub->getHandle(), 0, 0, &async_id));
	HEL_CHECK(helCloseDescriptor(posix_handle));
	
	helx::Pipe pipe;
	HelError connect_error;
	eventHub->waitForConnect(async_id, connect_error, pipe);
	HEL_CHECK(connect_error);
	posixPipe.initialize(frigg::move(pipe));
}

// __dso_handle is usually defined in crtbeginS.o
// Since we link with -nostdlib we have to manually define it here
__attribute__ (( visibility("hidden") )) void *__dso_handle;

extern "C" int main(int argc, char *argv[], char *env[]);

extern "C" void __mlibc_entry() {
	int result = main(0, NULL, NULL);
	exit(result);
}

