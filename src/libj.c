#include "libj_internal.h"
#include "libj_utils.h"

#include <libutf.h>

#include <stdio.h>
#include <string.h>

LibjError libsberror_to_libjerror(LibsbError err) {
    switch (err) {
    case LIBSB_ERROR_OK:
        return LIBJ_ERROR_OK;
    case LIBSB_ERROR_OUT_OF_MEMORY:
        return LIBJ_ERROR_OUT_OF_MEMORY;
    case LIBSB_ERROR_BAD_ARGUMENT:
        return LIBJ_ERROR_BAD_ARGUMENT;
    case LIBSB_ERROR_PRINTF:
        abort();
    case LIBSB_ERROR_UTF8:
        return LIBJ_ERROR_SYNTAX;
    }
    abort();
}

LibjError libiserror_to_libjerror(LibisError err) {
    switch (err) {
    case LIBIS_ERROR_OK:
        return LIBJ_ERROR_OK;
    case LIBIS_ERROR_OUT_OF_MEMORY:
        return LIBJ_ERROR_OUT_OF_MEMORY;
    case LIBIS_ERROR_BAD_ARGUMENT:
        abort();
    case LIBIS_ERROR_IO:
        return LIBJ_ERROR_IO;
    case LIBIS_ERROR_TOO_FAR:
    case LIBIS_ERROR_HANGING_BITS:
        abort();
    }
    abort();
}

LibjError libgberror_to_libjerror(LibgbError err) {
    switch (err) {
    case LIBGB_ERROR_OK:
        return LIBJ_ERROR_OK;
    case LIBGB_ERROR_BAD_ARGUMENT:
        abort();
    case LIBGB_ERROR_OUT_OF_MEMORY:
        return LIBJ_ERROR_OK;
    case LIBGB_ERROR_INDEX:
        abort();
    }
    abort();
}

LibjError libj_handle_internal_error(LibjError err) {
    switch (err) {
    case LIBJ_ERROR_OK:
        return LIBJ_ERROR_OK;
    case LIBJ_ERROR_OUT_OF_MEMORY:
        return LIBJ_ERROR_OUT_OF_MEMORY;
    case LIBJ_ERROR_BAD_TYPE:
    case LIBJ_ERROR_BAD_ARGUMENT:
        abort();
    case LIBJ_ERROR_NOT_FOUND:
        return LIBJ_ERROR_NOT_FOUND;
    case LIBJ_ERROR_PRECISION:
        return LIBJ_ERROR_PRECISION;
    case LIBJ_ERROR_SYNTAX:
        return LIBJ_ERROR_SYNTAX;
    case LIBJ_ERROR_UNKNOWN:
        return LIBJ_ERROR_UNKNOWN;
    case LIBJ_ERROR_IO:
        return LIBJ_ERROR_IO;
    case LIBJ_ERROR_ZERO:
        return LIBJ_ERROR_ZERO;
    }
    abort();
}

LibjError libj_start(Libj **libj) {
    LibjError err = LIBJ_ERROR_OK;
    Libj *libj_result = NULL;
    Libsb *libsb = NULL;
    Libgb *libgb = NULL;
    Libis *libis = NULL;
    if (!libj) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = ESB(libsb_start(&libsb));
    if (err) goto end;
    err = EGB(libgb_start(&libgb));
    if (err) goto end;
    err = EIS(libis_start(&libis));
    if (err) goto end;
    libj_result = malloc(sizeof(Libj));
    if (!libj_result) {
        goto end;
    }
    libj_result->error_string = NULL;
    libj_result->libsb = libsb;
    libsb = NULL;
    libj_result->libgb = libgb;
    libgb = NULL;
    libj_result->libis = libis;
    libis = NULL;
    *libj = libj_result;
    libj_result = NULL;
end:
    EIS(libis_finish(&libis));
    EGB(libgb_finish(&libgb));
    ESB(libsb_finish(&libsb));
    E(libj_finish(&libj_result));
    return err;
}

LibjError libj_finish(Libj **libj) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (!*libj) {
        goto end;
    }
    EIS(libis_finish(&(*libj)->libis));
    ESB(libsb_finish(&(*libj)->libsb));
    EGB(libgb_finish(&(*libj)->libgb));
    free((*libj)->error_string);
    free(*libj);
    *libj = NULL;
end:
    return err;
}

const char *libj_type_to_string(LibjType type) {
    switch (type) {
#define LIBJ_DECLARE_TYPE(name) case LIBJ_TYPE_##name: return #name;
    LIBJ_TYPES
#undef LIBJ_DECLARE_TYPE
    }
    abort();
}

LibjType libj_string_to_type(const char *string) {
#define LIBJ_DECLARE_TYPE(name) if (!strcmp(string, #name)) return LIBJ_TYPE_##name;
    LIBJ_TYPES
#undef LIBJ_DECLARE_TYPE
    abort();
}

const char *libj_error_to_string(LibjError error) {
    switch (error) {
#define LIBJ_DECLARE_ERROR(name, description) case LIBJ_ERROR_##name: return "LIBJ_ERROR_"#name;
    LIBJ_ERRORS
#undef LIBJ_DECLARE_ERROR
    }
    abort();
}

LibjError libj_string_to_error(const char *string) {
#define LIBJ_DECLARE_ERROR(name, description) if (!strcmp(string, "LIBJ_ERROR_"#name)) return LIBJ_ERROR_##name;
    LIBJ_ERRORS
#undef LIBJ_DECLARE_ERROR
    abort();
}

LibjError libj_type_of(Libj *libj, LibjJson *json, LibjType *type) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !type) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *type = json->type;
end:
    return err;
}

LibjError libj_free_json(Libj *libj, LibjJson **json) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*json) {
        switch ((*json)->type) {
        case LIBJ_TYPE_NULL:
            break;
        case LIBJ_TYPE_STRING:
        case LIBJ_TYPE_NUMBER:
            free((*json)->string.value);
            (*json)->string.value = NULL;
            (*json)->string.size = 0;
            break;
        case LIBJ_TYPE_BOOL:
            break;
        case LIBJ_TYPE_ARRAY:
            for (size_t i = 0; i < (*json)->array.size; ++i) {
                E(libj_free_json(libj, &(*json)->array.elements[i]));
                (*json)->array.elements[i] = NULL;
            }
            free((*json)->array.elements);
            (*json)->array.elements = NULL;
            (*json)->array.size = 0;
            break;
        case LIBJ_TYPE_OBJECT:
            for (size_t i = 0; i < (*json)->object.size; ++i) {
                LibjMember *member = &(*json)->object.members[i];
                free(member->name.value);
                E(libj_free_json(libj, &member->value));
            }
            free((*json)->object.members);
            (*json)->object.members = NULL;
            (*json)->object.size = 0;
            break;
        }
    }
    free(*json);
    *json = NULL;
end:
    return err;
}

static LibjError copy_string(Libj *libj, LibjString src, LibjString *dest) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !dest) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    dest->size = src.size;
    dest->value = malloc(src.size + 1);
    if (!dest->value) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    memcpy(dest->value, src.value, src.size);
    dest->value[dest->size] = '\0';
end:
    return err;
}

static LibjError copy_array(Libj *libj, LibjArray *source, LibjArray *target) {
    LibjError err = LIBJ_ERROR_OK;
    LibjArray result;
    size_t number_of_copied = 0;
    if (!source || !target) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result.size = source->size;
    result.elements = malloc(source->size * sizeof(LibjJson));
    if (!result.elements) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    for (; number_of_copied < source->size; ++number_of_copied) {
        err = E(libj_copy(libj, source->elements[number_of_copied], &result.elements[number_of_copied]));
        if (err) goto end;
    }
    *target = result;
    result.size = 0;
    result.elements = NULL;
    number_of_copied = 0;
end:
    for (; number_of_copied--;) {
        free(result.elements[number_of_copied]);
        E(libj_free_json(libj, &result.elements[number_of_copied]));
    }
    free(result.elements);
    return err;
}

static LibjError copy_object(Libj *libj, LibjObject *source, LibjObject *target) {
    LibjError err = LIBJ_ERROR_OK;
    LibjObject result;
    size_t number_of_copied = 0;
    if (!libj || !source || !target) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result.size = source->size;
    result.members = malloc(source->size * sizeof(LibjMember));
    if (!result.members) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    for (; number_of_copied < source->size; ++number_of_copied) {
        err = E(copy_string(libj, source->members[number_of_copied].name, &result.members[number_of_copied].name));
        if (err) goto end;
        err = E(libj_copy(libj, source->members[number_of_copied].value, &result.members[number_of_copied].value));
        if (err) goto end;
    }
    *target = result;
    result.size = 0;
    result.members = NULL;
    number_of_copied = 0;
end:
    for (; number_of_copied--;) {
        free(result.members[number_of_copied].name.value);
        E(libj_free_json(libj, &result.members[number_of_copied].value));
    }
    free(result.members);
    return err;
}

LibjError libj_copy(Libj *libj, LibjJson *source, LibjJson **target) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result = NULL;
    if (!libj || !source || !target) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibjJson));
    if (!result) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result->type = source->type;
    switch (source->type) {
    case LIBJ_TYPE_NULL:
        break;
    case LIBJ_TYPE_STRING:
    case LIBJ_TYPE_NUMBER:
        err = E(copy_string(libj, source->string, &result->string));
        if (err) goto end;
        break;
    case LIBJ_TYPE_BOOL:
        result->boolean = source->boolean;
        break;
    case LIBJ_TYPE_ARRAY:
        err = E(copy_array(libj, &source->array, &result->array));
        if (err) goto end;
        break;
    case LIBJ_TYPE_OBJECT:
        err = E(copy_object(libj, &source->object, &result->object));
        if (err) goto end;
        break;
    default:
        abort();
    }
    *target = result;
    result = NULL;
end:
    E(libj_free_json(libj, &result));
    return err;
}

LibjError libj_object_create(Libj *libj, LibjJson **json) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibjJson));
    if (!result) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result->type = LIBJ_TYPE_OBJECT;
    result->object.size = 0;
    result->object.members = NULL;
    *json = result;
    result = NULL;
end:
    free(result);
    return err;
}

LibjError libj_object_add_string(Libj *libj, LibjJson *json, const char *name, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_string_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add_integer(Libj *libj, LibjJson *json, const char *name, intmax_t value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_integer_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add_real(Libj *libj, LibjJson *json, const char *name, double value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_real_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add_number(Libj *libj, LibjJson *json, const char *name, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_number_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add_bool(Libj *libj, LibjJson *json, const char *name, bool value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_bool_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add_null(Libj *libj, LibjJson *json, const char *name) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_null_create(libj, &json_value));
    if (err) goto end;
    err = E(libj_object_add(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_add(Libj *libj, LibjJson *json, const char *name, LibjJson *value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_object_add_ex(libj, json, name, strlen(name), value));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_add_ex(Libj *libj, LibjJson *json, const char *name, size_t name_size, LibjJson *value) {
    LibjError err = LIBJ_ERROR_OK;
    char *name_copy = NULL;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_string_duplicate(libj, name, name_size, &name_copy));
    if (err) goto end;
    err = E(libj_copy(libj, value, &json_value));
    if (err) goto end;
    LibjMember *new_members = realloc(json->object.members, (json->object.size + 1) * sizeof(LibjMember));
    if (!new_members) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    new_members[json->object.size].name.value = name_copy;
    new_members[json->object.size].name.size = name_size;
    new_members[json->object.size].value = json_value;
    ++json->object.size;
    json->object.members = new_members;
    name_copy = NULL;
    json_value = NULL;
end:
    free(name_copy);
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_count_versions(Libj *libj, LibjJson *json, const char *name, size_t *nversions) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !nversions) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_object_count_versions_ex(libj, json, name, strlen(name), nversions));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_count_versions_ex(
        Libj *libj, LibjJson *json, const char *name, size_t name_size, size_t *nversions) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name || !nversions) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *nversions = 0;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    size_t count = 0;
    for (size_t i = 0; i < json->object.size; ++i) {
        const char *member_name = json->object.members[i].name.value;
        size_t member_name_size = json->object.members[i].name.size;
        if (member_name_size == name_size && !memcmp(member_name, name, name_size)) {
            ++count;
        }
    }
    *nversions = count;
end:
    return err;
}

LibjError libj_object_get_version(Libj *libj, LibjJson *json, LibjJson **value, const char *name, size_t version) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_object_get_version_ex(libj, json, value, name, strlen(name), version));
    if (err) goto end;
end:
    return err;
}

static LibjError object_get_version_index_ex(
        LibjJson *json, const char *name, size_t name_size, int version, size_t *index) {
    LibjError err = LIBJ_ERROR_OK;
    if (!json || !name || !index) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *index = 0;
    int count_versions_before_i = 0;
    for (size_t i = 0; i < json->object.size; ++i) {
        const char *member_name = json->object.members[i].name.value;
        size_t member_name_size = json->object.members[i].name.size;
        bool same_name = member_name_size == name_size && !memcmp(member_name, name, name_size);
        if (same_name && count_versions_before_i == version) {
            *index = i;
            break;
        }
        if (same_name) {
            ++count_versions_before_i;
        }
    }
end:
    return err;
}

LibjError libj_object_get_version_ex(Libj *libj, LibjJson *json, LibjJson **value, const char *name, size_t name_size,
                                     size_t version) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    size_t index;
    err = E(object_get_version_index_ex(json, name, name_size, version, &index));
    if (err) goto end;
    *value = json->object.members[index].value;
end:
    return err;
}

LibjError libj_object_get_string_(Libj *libj, LibjJson *json, char **value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_string_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_string_v(Libj *libj, LibjJson *json, char **value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    LibjJson *json_value;
    err = E(libj_object_get_v(libj, json, &json_value, name, args));
    if (err) goto end;
    err = E(libj_get_string(libj, json_value, value));
    if (err) goto end;
end:
    return err;
}

LibjError
libj_object_get_string_or_default_(Libj *libj, LibjJson *json, char **value, char *default_value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_string_or_default_v(libj, json, value, default_value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_string_or_default_v(Libj *libj, LibjJson *json, char **value, char *default_value,
                                              const char *name, va_list args) {
    LibjError err = libj_object_get_string_v(libj, json, value, name, args);
    if (LIBJ_ERROR_NOT_FOUND == err) {
        *value = default_value;
        err = LIBJ_ERROR_OK;
    }
    return err;
}

LibjError libj_object_get_integer_(Libj *libj, LibjJson *json, intmax_t *value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_integer_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_integer_v(Libj *libj, LibjJson *json, intmax_t *value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = 0;
    if (LIBJ_TYPE_OBJECT != json->type) {
        goto end;
    }
    LibjJson *json_value;
    err = E(libj_object_get_v(libj, json, &json_value, name, args));
    if (err) goto end;
    err = E(libj_get_integer(libj, json_value, value));
    if (err) goto end;
end:
    return err;
}

LibjError
libj_object_get_integer_or_default_(Libj *libj, LibjJson *json, intmax_t *value, intmax_t default_value,
                                    const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_integer_or_default_v(libj, json, value, default_value, name, args);
    va_end(args);
    return err;
}

LibjError
libj_object_get_integer_or_default_v(Libj *libj, LibjJson *json, intmax_t *value, intmax_t default_value,
                                     const char *name, va_list args) {
    LibjError err = libj_object_get_integer_v(libj, json, value, name, args);
    if (LIBJ_ERROR_NOT_FOUND == err) {
        *value = default_value;
        err = LIBJ_ERROR_OK;
    }
    return err;
}

LibjError libj_object_get_real_(Libj *libj, LibjJson *json, double *value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_real_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_real_v(Libj *libj, LibjJson *json, double *value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = 0;
    if (LIBJ_TYPE_OBJECT != json->type) {
        goto end;
    }
    LibjJson *json_value;
    err = E(libj_object_get_v(libj, json, &json_value, name, args));
    if (err) goto end;
    err = E(libj_get_real(libj, json_value, value));
    if (err) goto end;
end:
    return err;
}

LibjError
libj_object_get_real_or_default_(Libj *libj, LibjJson *json, double *value, double default_value,
                                 const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_real_or_default_v(libj, json, value, default_value, name, args);
    va_end(args);
    return err;
}

LibjError
libj_object_get_real_or_default_v(Libj *libj, LibjJson *json, double *value, double default_value,
                                  const char *name, va_list args) {
    LibjError err = libj_object_get_real_v(libj, json, value, name, args);
    if (LIBJ_ERROR_NOT_FOUND == err) {
        *value = default_value;
        err = LIBJ_ERROR_OK;
    }
    return err;
}

LibjError libj_object_get_number_(Libj *libj, LibjJson *json, char **value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_number_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_number_v(Libj *libj, LibjJson *json, char **value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    LibjJson *json_value;
    err = E(libj_object_get_v(libj, json, &json_value, name, args));
    if (err) goto end;
    err = E(libj_get_number(libj, json_value, value));
    if (err) goto end;
end:
    return err;
}

LibjError
libj_object_get_number_or_default_(Libj *libj, LibjJson *json, char **value, char *default_value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_number_or_default_v(libj, json, value, default_value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_number_or_default_v(Libj *libj, LibjJson *json, char **value, char *default_value,
                                              const char *name, va_list args) {
    LibjError err = libj_object_get_number_v(libj, json, value, name, args);
    if (LIBJ_ERROR_NOT_FOUND == err) {
        *value = default_value;
        err = LIBJ_ERROR_OK;
    }
    return err;
}

LibjError libj_object_get_bool_(Libj *libj, LibjJson *json, bool *value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_bool_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_bool_v(Libj *libj, LibjJson *json, bool *value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = false;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    LibjJson *json_value;
    err = E(libj_object_get_v(libj, json, &json_value, name, args));
    if (err) goto end;
    err = E(libj_get_bool(libj, json_value, value));
    if (err) goto end;
end:
    return err;
}

LibjError
libj_object_get_bool_or_default_(Libj *libj, LibjJson *json, bool *value, bool default_value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_bool_or_default_v(libj, json, value, default_value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_bool_or_default_v(Libj *libj, LibjJson *json, bool *value, bool default_value,
                                            const char *name, va_list args) {
    LibjError err = libj_object_get_bool_v(libj, json, value, name, args);
    if (LIBJ_ERROR_NOT_FOUND == err) {
        *value = default_value;
        err = LIBJ_ERROR_OK;
    }
    return err;
}

LibjError libj_object_get_ex(Libj *libj, LibjJson *json, LibjJson **value, const char *name, size_t name_size) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    size_t number_of_versions;
    err = E(libj_object_count_versions_ex(libj, json, name, name_size, &number_of_versions));
    if (err) goto end;
    if (!number_of_versions) {
        err = LIBJ_ERROR_NOT_FOUND;
        goto end;
    }
    err = E(libj_object_get_version_ex(libj, json, value, name, name_size, number_of_versions - 1));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_get_(Libj *libj, LibjJson *json, LibjJson **value, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_get_v(libj, json, value, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_get_v(Libj *libj, LibjJson *json, LibjJson **value, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    while (name) {
        err = E(libj_object_get_ex(libj, json, &json, name, strlen(name)));
        if (err) goto end;
        name = va_arg(args, char *);
    }
    *value = json;
end:
    return err;
}

LibjError libj_object_get_size(Libj *libj, LibjJson *json, size_t *size) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !size) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *size = 0;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *size = json->object.size;
end:
    return err;
}

LibjError libj_object_set_null(Libj *libj, LibjJson *json, const char *name) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT == json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_null_create(libj, &json_value));
    if (err) goto end;
    err = E(libj_object_set(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_set_bool(Libj *libj, LibjJson *json, const char *name, bool value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT == json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_bool_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_set(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_set_integer(Libj *libj, LibjJson *json, const char *name, intmax_t value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT == json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_integer_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_set(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_set_real(Libj *libj, LibjJson *json, const char *name, double value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT == json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_real_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_set(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_set_string(Libj *libj, LibjJson *json, const char *name, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_string_create(libj, &json_value, value));
    if (err) goto end;
    err = E(libj_object_set(libj, json, name, json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_object_set(Libj *libj, LibjJson *json, const char *name, LibjJson *value) {
    return libj_object_set_ex(libj, json, name, strlen(name), value);
}

LibjError libj_object_set_ex(Libj *libj, LibjJson *json, const char *name, size_t name_size, LibjJson *value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_object_remove_all_ex(libj, json, name, name_size));
    if (err) goto end;
    err = E(libj_object_add_ex(libj, json, name, name_size, value));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_member_at(Libj *libj, LibjJson *json, size_t i, const char **name, LibjJson **value) {
    size_t name_size;
    return libj_object_member_at_ex(libj, json, i, name, &name_size, value);
}

LibjError
libj_object_member_at_ex(Libj *libj, LibjJson *json, size_t i, const char **name, size_t *name_size, LibjJson **value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name || !name_size || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *name = NULL;
    *name_size = 0;
    *value = NULL;
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    if (json->object.size <= i) {
        err = LIBJ_ERROR_NOT_FOUND;
        goto end;
    }
    LibjMember member = json->object.members[i];
    *name = member.name.value;
    *name_size = member.name.size;
    *value = member.value;
end:
    return err;
}

LibjError libj_object_remove_(Libj *libj, LibjJson *json, const char *name, ...) {
    va_list args;
    va_start(args, name);
    LibjError err = libj_object_remove_v(libj, json, name, args);
    va_end(args);
    return err;
}

LibjError libj_object_remove_v(Libj *libj, LibjJson *json, const char *name, va_list args) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    char *next_name = va_arg(args, char *);
    while (next_name) {
        err = E(libj_object_get_(libj, json, &json, name));
        if (err) goto end;
        next_name = va_arg(args, char *);
    }
    size_t number_of_versions;
    err = E(libj_object_count_versions(libj, json, name, &number_of_versions));
    if (err) goto end;
    err = E(libj_object_remove_ex(libj, json, name, strlen(name), number_of_versions - 1));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_remove_ex(Libj *libj, LibjJson *json, const char *name, size_t name_size, size_t version) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    size_t index;
    err = E(object_get_version_index_ex(json, name, name_size, version, &index));
    if (err) goto end;
    err = E(libj_object_remove_at(libj, json, index));
    if (err) goto end;
end:
    return err;
}

LibjError libj_object_remove_at(Libj *libj, LibjJson *json, size_t index) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    if (json->object.size <= index) {
        err = LIBJ_ERROR_NOT_FOUND;
        goto end;
    }
    LibjMember member_to_remove = json->object.members[index];
    void *dst = &json->object.members[index];
    void *src = &json->object.members[index + 1];
    size_t number_of_bytes = sizeof(LibjMember) * (json->object.size - index - 1);
    memmove(dst, src, number_of_bytes);
    --json->object.size;
    LibjMember *new_members = realloc(json->object.members, sizeof(LibjMember) * json->object.size);
    if (!new_members && json->object.size) {
        ++json->object.size;
        dst = &json->object.members[index + 1];
        src = &json->object.members[index];
        number_of_bytes = sizeof(LibjMember) * (json->object.size - index - 1);
        memmove(dst, src, number_of_bytes);
        json->object.members[index] = member_to_remove;
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    json->object.members = new_members;
    E(libj_free_json(libj, &member_to_remove.value));
    free(member_to_remove.name.value);
end:
    return err;
}

LibjError libj_object_remove_all_ex(Libj *libj, LibjJson *json, const char *name, size_t name_size) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !name) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_OBJECT != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    for (size_t i = 0; i < json->object.size; ++i) {
        const char *member_name = json->object.members[i].name.value;
        size_t member_name_size = json->object.members[i].name.size;
        if (member_name_size == name_size && !memcmp(member_name, name, name_size)) {
            err = E(libj_object_remove_at(libj, json, i));
            if (err) goto end;
        }
    }
end:
    return err;
}

LibjError libj_array_create(Libj *libj, LibjJson **json) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *json = malloc(sizeof(LibjJson));
    if (!*json) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    (*json)->type = LIBJ_TYPE_ARRAY;
    (*json)->array.size = 0;
    (*json)->array.elements = NULL;
end:
    return err;
}

static LibjError array_add(LibjJson *json, LibjJson **element) {
    LibjError err = LIBJ_ERROR_OK;
    if (!json || !element || !*element) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    LibjJson **new_elements = realloc(json->array.elements, (json->array.size + 1) * sizeof(LibjJson *));
    if (!new_elements) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    new_elements[json->array.size] = *element;
    json->array.elements = new_elements;
    ++json->array.size;
    *element = NULL;
end:
    return err;
}

LibjError libj_array_add_string(Libj *libj, LibjJson *json, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_string_create(libj, &json_value, value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
    end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add_integer(Libj *libj, LibjJson *json, intmax_t value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_integer_create(libj, &json_value, value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
    end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add_real(Libj *libj, LibjJson *json, double value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_real_create(libj, &json_value, value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add_number(Libj *libj, LibjJson *json, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_number_create(libj, &json_value, value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add_bool(Libj *libj, LibjJson *json, bool value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_bool_create(libj, &json_value, value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add_null(Libj *libj, LibjJson *json) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *json_value;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_null_create(libj, &json_value));
    if (err) goto end;
    err = E(array_add(json, &json_value));
    if (err) goto end;
end:
    E(libj_free_json(libj, &json_value));
    return err;
}

LibjError libj_array_add(Libj *libj, LibjJson *json, LibjJson *element) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *element_copy = NULL;
    if (!libj || !json || !element) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    err = E(libj_copy(libj, element, &element_copy));
    if (err) goto end;
    LibjJson **new_elements = realloc(json->array.elements, (json->array.size + 1) * sizeof(LibjJson *));
    if (!new_elements) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    json->array.elements = new_elements;
    json->array.elements[json->array.size] = element_copy;
    ++json->array.size;
    element_copy = NULL;
end:
    E(libj_free_json(libj, &element_copy));
    return err;
}

LibjError libj_array_get_size(Libj *libj, LibjJson *json, size_t *size) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !size) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *size = 0;
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *size = json->array.size;
end:
    return err;
}

LibjError libj_array_element_at(Libj *libj, LibjJson *json, size_t i, LibjJson **element) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !element) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *element = NULL;
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    if (json->array.size <= i) {
        err = LIBJ_ERROR_NOT_FOUND;
        goto end;
    }
    *element = json->array.elements[i];
end:
    return err;
}

LibjError libj_array_remove_at(Libj *libj, LibjJson *json, size_t index) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_ARRAY != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    if (json->array.size <= index) {
        err = LIBJ_ERROR_NOT_FOUND;
        goto end;
    }
    LibjJson *json_to_remove = json->array.elements[index];
    void *dst = &json->array.elements[index];
    void *src = &json->array.elements[index + 1];
    size_t number_of_bytes = sizeof(LibjJson *) * (json->array.size - index - 1);
    memmove(dst, src, number_of_bytes);
    --json->array.size;
    LibjJson **new_elements = realloc(json->array.elements, sizeof(LibjJson *) * json->array.size);
    if (!new_elements && json->array.size) {
        ++json->array.size;
        dst = &json->array.elements[index + 1];
        src = &json->array.elements[index];
        number_of_bytes = sizeof(LibjJson *) * (json->array.size - index - 1);
        memmove(dst, src, number_of_bytes);
        json->array.elements[index] = json_to_remove;
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    json->array.elements = new_elements;
    E(libj_free_json(libj, &json_to_remove));
end:
    return err;
}

LibjError libj_get_integer(Libj *libj, LibjJson *json, intmax_t *value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = 0;
    if (LIBJ_TYPE_NUMBER != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = 0;
    char *endptr;
    *value = strtoimax(json->string.value, &endptr, 10);
    if (endptr != json->string.value + json->string.size) {
        err = LIBJ_ERROR_SYNTAX;
        goto end;
    }
end:
    return err;
}

LibjError libj_get_real(Libj *libj, LibjJson *json, double *value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (LIBJ_TYPE_NUMBER != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = 0;
    char *endptr;
    *value = strtod(json->string.value, &endptr);
    if (endptr != json->string.value + json->string.size) {
        err = LIBJ_ERROR_SYNTAX;
        goto end;
    }
end:
    return err;
}

LibjError libj_get_number(Libj *libj, LibjJson *json, char **value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = 0;
    if (LIBJ_TYPE_NUMBER != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = json->string.value;
end:
    return err;
}

LibjError libj_get_bool(Libj *libj, LibjJson *json, bool *value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = false;
    if (LIBJ_TYPE_BOOL != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = json->boolean;
end:
    return err;
}

LibjError libj_get_string(Libj *libj, LibjJson *json, char **value) {
    size_t value_size;
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libj_get_string_ex(libj, json, value, &value_size));
    if (err) goto end;
    if (strlen(*value) != value_size) {
        err = LIBJ_ERROR_ZERO;
        goto end;
    }
end:
    return err;
}

LibjError libj_get_string_ex(Libj *libj, LibjJson *json, char **value, size_t *value_size) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json || !value || !value_size) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *value = "";
    *value_size = 0;
    if (LIBJ_TYPE_STRING != json->type) {
        err = LIBJ_ERROR_BAD_TYPE;
        goto end;
    }
    *value = json->string.value;
    *value_size = json->string.size;
end:
    return err;
}

LibjError libj_string_create(Libj *libj, LibjJson **json, const char *value) {
    return libj_string_create_ex(libj, json, value, strlen(value));
}

LibjError libj_string_create_ex(Libj *libj, LibjJson **json, const char *value, size_t value_size) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result = NULL;
    char *value_copy = NULL;
    if (!libj || !json || !value) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibjJson));
    if (!result) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    err = E(libj_string_duplicate(libj, value, value_size, &value_copy));
    if (err) goto end;
    result->type = LIBJ_TYPE_STRING;
    result->string.size = value_size;
    result->string.value = value_copy;
    *json = result;
    result = NULL;
    value_copy = NULL;
end:
    free(result);
    free(value_copy);
    return err;
}

LibjError libj_integer_create(Libj *libj, LibjJson **json, intmax_t value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result = NULL;
    LibsbBuilder *builder = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibjJson));
    if (!result) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    err = ESB(libsb_create(libj->libsb, &builder));
    if (err) goto end;
    err = ESB(libsb_append(libj->libsb, builder, "%"PRIiMAX, value));
    if (err) goto end;
    err = ESB(libsb_destroy_into(libj->libsb, &builder, &result->string.value, &result->string.size));
    if (err) goto end;
    result->type = LIBJ_TYPE_NUMBER;
    *json = result;
    result = NULL;
end:
    ESB(libsb_destroy(libj->libsb, &builder));
    free(result);
    return err;
}

LibjError libj_real_create(Libj *libj, LibjJson **json, double value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result = NULL;
    LibsbBuilder *builder = NULL;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibjJson));
    if (!result) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    err = ESB(libsb_create(libj->libsb, &builder));
    if (err) goto end;
    err = ESB(libsb_append(libj->libsb, builder, "%lg", value));
    if (err) goto end;
    err = ESB(libsb_destroy_into(libj->libsb, &builder, &result->string.value, &result->string.size));
    if (err) goto end;
    result->type = LIBJ_TYPE_NUMBER;
    *json = result;
    result = NULL;
end:
    ESB(libsb_destroy(libj->libsb, &builder));
    free(result);
    return err;
}

LibjError libj_number_create(Libj *libj, LibjJson **json, const char *value) {
    LibjError err = LIBJ_ERROR_OK;
    LibjJson *result;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    const char *error_string;
    err = E(libj_from_string(libj, &result, value, &error_string));
    if (err) goto end;
    if (result->type != LIBJ_TYPE_NUMBER) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *json = result;
    result = NULL;
end:
    E(libj_free_json(libj, &result));
    return err;
}

LibjError libj_bool_create(Libj *libj, LibjJson **json, bool value) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *json = malloc(sizeof(LibjJson));
    if (!*json) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    (*json)->type = LIBJ_TYPE_BOOL;
    (*json)->boolean = value;
end:
    return err;
}

LibjError libj_null_create(Libj *libj, LibjJson **json) {
    LibjError err = LIBJ_ERROR_OK;
    if (!libj || !json) {
        err = LIBJ_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *json = malloc(sizeof(LibjJson));
    if (!*json) {
        err = LIBJ_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    (*json)->type = LIBJ_TYPE_NULL;
end:
    return err;
}
