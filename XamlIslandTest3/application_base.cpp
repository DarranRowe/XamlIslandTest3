#include "pch.h"
#include "application_base.h"

//Destroys the stored application_base pointer.
void application_base::close()
{
	application_base::destroy_application();
}

//Obtains the instance of application_base.
application_base &application_base::get_stored_application()
{
	auto &storage = internal_get_stored_application();

	_ASSERTE(storage.application != nullptr);
	return *storage.application.get();
}

//Sets the instance of application_base from the pointer passed in.
//This comes from the factory function of the derived class.
void application_base::set_application_instance(application_base *ptr)
{
	_ASSERTE(ptr != nullptr);
	auto &storage = internal_get_stored_application();

	_ASSERTE(storage.application == nullptr);
	storage.application.reset(ptr);
}

bool application_base::contains_application()
{
	return (internal_get_stored_application().application != nullptr);
}

application_base::application_store &application_base::internal_get_stored_application()
{
	static application_store storage{};

	return storage;
}

void application_base::destroy_application()
{
	auto &storage = internal_get_stored_application();

	storage.application.reset();
}