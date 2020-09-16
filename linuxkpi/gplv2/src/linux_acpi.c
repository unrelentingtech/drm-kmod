#include <linux/acpi.h>
#include <acpi/button.h>
#include <linux/pci.h>


#define	_COMPONENT	ACPI_OS_SERVICES

ACPI_MODULE_NAME("linux_acpi")


char empty_zero_page[PAGE_SIZE] __aligned(PAGE_SIZE);


#define INVALID_ACPI_HANDLE	((acpi_handle)empty_zero_page)

extern acpi_handle acpi_lid_handle;

#define acpi_handle_warn(handle, fmt, ...)

acpi_status
AcpiGetData(acpi_handle obj_handle, acpi_object_handler handler, void **data);


extern acpi_status
AcpiEvaluateObjectTyped(acpi_handle handle,
			acpi_string pathname,
			struct acpi_object_list *external_params,
			struct acpi_buffer *return_buffer,
			acpi_object_type return_type);

extern acpi_status
AcpiEvaluateObject(acpi_handle handle,
		   acpi_string pathname,
		   struct acpi_object_list *external_params,
		   struct acpi_buffer *return_buffer);
extern acpi_status
AcpiWalkNamespace(acpi_object_type type,
		    acpi_handle start_object,
		    u32 max_depth,
		    acpi_walk_callback
		    descending_callback,
		    acpi_walk_callback
		    ascending_callback,
		    void *context,
		    void **return_value);


acpi_status
AcpiGetName(acpi_handle handle, u32 name_type, struct acpi_buffer * buffer);

acpi_status
AcpiGetHandle(acpi_handle parent,
    acpi_string pathname, acpi_handle * ret_handle);

acpi_status
AcpiGetTable(acpi_string signature, u32 instance, struct acpi_table_header **out_table);




extern const char *
AcpiFormatException(acpi_status status);

union acpi_object *
acpi_evaluate_dsm(acpi_handle handle, const u8 *uuid, int rev, int func,
		  union acpi_object *argv4)
{
	acpi_status ret;
	struct acpi_buffer buf = {ACPI_ALLOCATE_BUFFER, NULL};
	union acpi_object params[4];
	struct acpi_object_list input = {
		.Count = 4,
		.Pointer = params,
	};

	params[0].Type = ACPI_TYPE_BUFFER;
	params[0].Buffer.Length = 16;
	params[0].Buffer.Pointer = (char *)uuid;
	params[1].Type = ACPI_TYPE_INTEGER;
	params[1].Integer.Value = rev;
	params[2].Type = ACPI_TYPE_INTEGER;
	params[2].Integer.Value = func;
	if (argv4) {
		params[3] = *argv4;
	} else {
		params[3].Type = ACPI_TYPE_PACKAGE;
		params[3].Package.Count = 0;
		params[3].Package.Elements = NULL;
	}

	ret = acpi_evaluate_object(handle, "_DSM", &input, &buf);
	if (ACPI_SUCCESS(ret))
		return (union acpi_object *)buf.Pointer;

	if (ret != AE_NOT_FOUND)
		acpi_handle_warn(handle,
				"failed to evaluate _DSM (0x%x)\n", ret);

	return NULL;
}

/**
 * acpi_check_dsm - check if _DSM method supports requested functions.
 * @handle: ACPI device handle
 * @uuid: UUID of requested functions, should be 16 bytes at least
 * @rev: revision number of requested functions
 * @funcs: bitmap of requested functions
 *
 * Evaluate device's _DSM method to check whether it supports requested
 * functions. Currently only support 64 functions at maximum, should be
 * enough for now.
 */
bool
acpi_check_dsm(acpi_handle handle, const u8 *uuid, int rev, u64 funcs)
{
	int i;
	u64 mask = 0;
	union acpi_object *obj;

	if (funcs == 0)
		return false;

	obj = acpi_evaluate_dsm(handle, uuid, rev, 0, NULL);
	if (!obj)
		return false;

	/* For compatibility, old BIOSes may return an integer */
	if (obj->Type == ACPI_TYPE_INTEGER)
		mask = obj->Integer.Value;
	else if (obj->Type == ACPI_TYPE_BUFFER)
		for (i = 0; i < obj->Buffer.Length && i < 8; i++)
			mask |= (((u64)obj->Buffer.Pointer[i]) << (i * 8));
	ACPI_FREE(obj);

	/*
	 * Bit 0 indicates whether there's support for any functions other than
	 * function 0 for the specified UUID and revision.
	 */
	if ((mask & 0x1) && (mask & funcs) == funcs)
		return true;

	return false;
}

static LINUX_LIST_HEAD(acpi_device_del_list);
static DEFINE_MUTEX(acpi_device_del_lock);

#ifdef CONFIG_ACPI_SLEEP
u32 linuxkpi_acpi_target_sleep_state = ACPI_STATE_S0;

u32 acpi_target_system_state(void)
{

        return linuxkpi_acpi_target_sleep_state;
}
#endif
