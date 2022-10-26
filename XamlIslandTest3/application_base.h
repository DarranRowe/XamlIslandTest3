#pragma once

#ifndef _MEMORY_
#include <memory>
#endif

//The singleton portion of the application class.
//Used to store the instance of the application_base object
//and control the lifetime of the object.
class application_base
{
public:
	//Required to be virtual since the destruction of this object will
	//happen through the application_base pointer.
	virtual ~application_base() = default;
	//Destroys the instance of the application class stored statically.
	virtual void close();

protected:
	static application_base &get_stored_application();
	static void set_application_instance(application_base *);
	static bool contains_application();

private:
	struct application_store
	{
		std::unique_ptr<application_base> application;
	};

	static application_store &internal_get_stored_application();
	static void destroy_application();
};