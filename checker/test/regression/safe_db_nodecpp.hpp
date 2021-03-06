

/*
 * This file contain type names and functions names that are declared as 'safe'
 * under Rule S8.
 * At this moment only names are of interest, we don't care about arguments,
 * return type or template parameters. This simplifies this file a lot.
 * 
 */

namespace nodecpp {

	void wait_for_all();
	void await_function();
	void no_await_function();

	class awaitable {
		void await_ready();
		void await_resume();
		void await_suspend();

	};

	class promise_type_struct {
		void yield_value();
		void return_void();
	};

	class SrvMember
	{
		void onEvent();
	};


namespace safememory {
	void make_owning();


	// osn ptrs are hardcoded with special safety rules
	// they must not be included here
	// class owning_ptr_impl;
	// class owning_ptr_no_checks;
	// class soft_ptr_impl;
	// class soft_ptr_no_checks;
	// class soft_this_ptr_impl;
	// class soft_this_ptr_no_checks;
	// class naked_ptr_impl;
	// class naked_ptr_no_checks;
}
}


