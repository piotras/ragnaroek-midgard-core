/*
 * Copyright (c) 2005 Jukka Zitting <jz@yukatan.fi>
 * Copyright (c) 2005 Piotr Pokora <piotr.pokora@infoglob.pl>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef MIDGARD_QUERY_BUILDER_H
#define MIDGARD_QUERY_BUILDER_H

/**
 * \defgroup qb Midgard Query Builder
 *
 * The Midgard Query Builder allows an application to construct and execute
 * queries against the Midgard database. The main steps of using the query
 * builder are:
 *
 * \li create a query builder instance
 * \li add constraints and other options to the query
 * \li execute the query
 *
 * Each query is represented by an instance of the opaque MidgardQueryBuilder
 * structure. The \c midgard_query_builder functions can be used to manipulate
 * the queries.
 */

#include "midgard/midgard_connection.h"

/* convention macros */
#define MIDGARD_TYPE_QUERY_BUILDER (midgard_query_builder_get_type())
#define MIDGARD_QUERY_BUILDER(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_QUERY_BUILDER, MidgardQueryBuilder))
#define MIDGARD_QUERY_BUILDER_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_QUERY_BUILDER, MidgardQueryBuilderClass))
#define MIDGARD_IS_QUERY_BUILDER(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_QUERY_BUILDER))
#define MIDGARD_IS_QUERY_BUILDER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_QUERY_BUILDER))
#define MIDGARD_QUERY_BUILDER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_QUERY_BUILDER, MidgardQueryBuilderClass))

/**
 * \ingroup qb
 * The opaque Midgard Query Builder type. Instances are used to track
 * the state of a query builder.
 */
typedef struct MidgardQueryBuilder MidgardQueryBuilder;
typedef struct _MidgardQueryBuilderPrivate MidgardQueryBuilderPrivate;

/* FIXME! Implement MidgardQueryBuilderPrivate! */
struct MidgardQueryBuilder {
	GObject parent;

	MidgardQueryBuilderPrivate *priv;
};

/**
 * \ingroup qb
 * The opaque Midgard Query Builder class.
 */ 
typedef struct MidgardQueryBuilderClass MidgardQueryBuilderClass;

/** 
 * \ingroup qb
 * The class structure for Midgard Query Builder class.
 */
struct MidgardQueryBuilderClass{
	GObjectClass parent;

	void (*set_lang) (
		MidgardQueryBuilder *builder, gint lang);

	void (*unset_languages) (
		MidgardQueryBuilder *builder);
	
	gboolean (*add_constraint) (
		MidgardQueryBuilder *builder,
		const gchar *name, const gchar *op, const GValue *value);
	
	gboolean (*begin_group) (
		MidgardQueryBuilder *builder, const gchar *type);

	gboolean (*end_group) (
		MidgardQueryBuilder *builder);

	gboolean (*add_order) (
		MidgardQueryBuilder *builder, 
		const gchar *name, const gchar *dir);

	void (*set_offset) (
		MidgardQueryBuilder *builder, guint offset);

	void (*set_limit) (
		MidgardQueryBuilder *builder, guint limit);

	GObject **(*execute) (
		MidgardQueryBuilder *builder, MidgardTypeHolder *holder);

	guint (*count) (
		MidgardQueryBuilder *builder);
};

/**
 * \ingroup qb
 *
 * Returns MidgardQueryBuilder type.
 * Registers the type as  a fundamental GType unless already registered.
 */
extern GType midgard_query_builder_get_type(void);

/**
 * \ingroup qb
 * Creates a new query builder for the given Midgard connection and
 * MgdSchema class. Returns \c NULL if the named MgdSchema class is not found.
 *
 * \param     mgd       Midgard connection
 * \param[in] classname MgdSchema class name
 * \return query builder, or \c NULL if the named class is not found
 */
extern MidgardQueryBuilder *midgard_query_builder_new(
        midgard *mgd, const gchar *classname);

/**
 * \ingroup qb
 * Releases the given query builder and all associated resources.
 *
 * \param builder query builder
 */
extern void midgard_query_builder_free(MidgardQueryBuilder *builder);

/**
 * \ingroup qb
 * Sets internal lang used by Query Builder.
 * 
 * \param     builder query builder
 * \param[in] lang    Sets object's lang
 *
 * SQL query used by Query Builder will use set language explicitly.
 * Objects in other languages will be not queried from database.
 */
extern void midgard_query_builder_set_lang(
        MidgardQueryBuilder *builder, gint lang);

extern void midgard_query_builder_toggle_read_only(
	MidgardQueryBuilder *builder, gboolean toggle);

/**
 * \ingroup qb
 *
 * Unset all languages used by given Query Builder instance.
 *
 * \param builder MidgardQueryBuidler instance.
 *
 * All languages ( current and default one ) are unset for QB instance.
 * SQL query is executed without language context.
 */
extern void midgard_query_builder_unset_languages(
	MidgardQueryBuilder *builder);

/**
 * \ingroup qb
 * Adds a constraint to the given query builder. The constraint is
 * expressed as a triple of a field name, a comparison operator, and
 * a scalar comparison value. The field name referes to a property of
 * the queried Midgard object class. The comparison operator is a
 * string representation of the requested comparison. Available operators
 * are =, <>, <, >, <=, >=, and LIKE. The given scalar value is copied and
 * converted into the property type before comparison.
 *
 * This method returns \c TRUE if the given constraint is valid (refers to
 * a valid field, uses a valid operator, and contains a valid value).
 * Otherwise this method returns \c FALSE and does not change the state
 * of the query builder.
 *
 * \param     builder query builder
 * \param[in] name    name of the field used for the constraint
 * \param[in] op      comparison operator
 * \param[in] value   scalar value used in comparison
 * \return \c TRUE if the constraint is valid, \c FALSE otherwise
 */
extern gboolean midgard_query_builder_add_constraint(
        MidgardQueryBuilder *builder,
        const gchar *name, const gchar *op, const GValue *value);

/**
 * \ingroup qb
 *
 * Adds named property constraint to the given query builder.
 * Unlike add_constraint method, this one accepts property name
 * instead of scalar value. The difference is that with add_constraint
 * method you can compare property with particular value, while using 
 * add_constraint_with_property method you can compare two different 
 * properties without need to know their values. 
 * For example, you should use this method if you want to select only 
 * those objects which has been revised after publication time, and particular 
 * date doesn't matter: (metadata.revised > metadata.published).
 *
 * \param builder, MidgardQueryBuilder object
 * \param property_a, name of the first property
 * \param op, comparison operator
 * \param property_b, a name of the second property
 * \return \c TRUE if properties' names are valid, \c FALSE otherwise
 */
extern gboolean midgard_query_builder_add_constraint_with_property(
	MidgardQueryBuilder *builder, const gchar *property_a,
	const gchar *op, const gchar *property_b);

/**
 * \ingroup qb
 * Starts a constraint group of the given type. A conjunctive constraint
 * group (type \c AND) requires that all component constraints match the
 * queried objects, while a disjunctive group (type \c OR) requires just
 * one of the component constraints to match.
 *
 * \param     builder query builder
 * \param[in] type    group type (\c AND or \c OR)
 * \return \c TRUE if the type is valid (\c AND or \c OR), \c FALSE otherwise
 */
extern gboolean midgard_query_builder_begin_group(
        MidgardQueryBuilder *builder, const gchar *type);

/**
 * \ingroup qb
 * Closes the most recently opened constraint group. The client should
 * ensure proper nesting by closing all constraint groups before the
 * containing query is executed.
 *
 * \param builder query builder
 * \return \c TRUE if a constraint group was closed, or \c FALSE if no
 *         open constraint groups were found
 */
extern gboolean midgard_query_builder_end_group(MidgardQueryBuilder *builder);

/**
 * \ingroup qb
 * Adds an ordering constraint to the query. An ordering constraint consists
 * of a property name and a sort direction. The objects returned by this
 * query will be sorted by the given property in the given direction
 * (ascending or descending). Multiple ordering constraints are applied in
 * the order they were added.
 *
 * \param     builder query builder
 * \param[in] name    name of the order property
 * \param[in] dir     sort direction (\c ASC or \c DESC)
 * \return \c TRUE if the ordering constraint is valid, \c FALSE otherwise
 */
extern gboolean midgard_query_builder_add_order(
        MidgardQueryBuilder *builder, const gchar *name, const gchar *dir);

/**
 * \ingroup qb
 * Sets the start offset of the objects to return when the query is executed.
 * The start offset is applied after all the matching objects have been
 * identified and sorted according to the given ordering constraints. The
 * first \c offset objects are skipped and only the remaining (if any) objects
 * are returned to the client.
 *
 * Setting a start offset is normally only reasonable when one or more
 * ordering constraints are applied to the query. A start offset is usually
 * accompanied by a limit setting.
 *
 * \param     builder query builder
 * \param[in] offset  query offset
 */
extern void midgard_query_builder_set_offset(
        MidgardQueryBuilder *builder, guint offset);

/**
 * \ingroup qb
 * Sets the maximum number of objects to return when the query is executed.
 * A query will by default return all matching objects, but the limit setting
 * can be used to restrict the potentially large number of returned objects.
 * The limit is applied only after the matching objects have been identified
 * and sorted and after the optional start offset has been applied.
 *
 * Setting a limit on the number of returned objects is normally only
 * reasonable when one or more ordering constraints and optionally an offset
 * setting are applied to the query.
 *
 * \param     builder query builder
 * \param[in] limit   query limit
 */
extern void midgard_query_builder_set_limit(
        MidgardQueryBuilder *builder, guint limit);

/**
 * \ingroup qb
 * Executes the built query. The matched recors are returned as full
 * MgdObjects.
 *
 * \param      builder   query builder
 * \param[out] holder place to store the number of returned objects
 * \return objects matched by the query terminated by NULL.
 *
 * Number of returned objects are available as holder->elements member.
 * You should free holder's struct when no longer needed.
 * Holder is also optionally.
 *
 */
extern GObject **midgard_query_builder_execute(
        MidgardQueryBuilder *builder, MidgardTypeHolder *holder);

/**
 * \ingroup qb
 * Returns the number of objects that this query would return when executed
 * without limit or start offset settings.
 *
 * \warning At the moment this function is implemented by simply calling
 * midgard_query_builder_execute and discarding all the returned objects.
 * An optimized version of this function is however being planned, so
 * applications should feel free to start using this function.
 *
 * \return number of objects matched by this query
 */
extern guint midgard_query_builder_count(MidgardQueryBuilder *builder); 


/**
 * \ingroup qb
 * Joins type's properties' fields in query.
 *
 * \param builder Midgard Query Builder object's instance
 * \param prop property which is a memeber of builder type
 * \param jobject typename which property should be joined
 * \param jprop property which should be joined
 * 
 * \return \c TRUE if properties are valid for defined types and query can be built
 * , \c FALSE otherwise
 * 
 */
gboolean midgard_query_builder_join(
		MidgardQueryBuilder *builder, const gchar *prop,
		const gchar *jobject, const gchar *jprop);
/**
 * \ingroup qb
 *
 * Executes the built query. The matched recors are returned as object's guid.
 * Look at midgard_query_builder_execute which returns full objects.
 *
 * \param builder Midgard Query Builder object's instance
 *
 * \return Glist 
 *
 * Data member of returned list contains object's guid. Caller is responsible 
 * to free memory allocated by all list's data and list itself.
 */ 
GList *midgard_query_builder_get_guid(MidgardQueryBuilder *builder);

/**
 * \ingroup qb
 *
 * Returns type name of the type which is currently used by Query Builder.
 * 
 * This function should be used on language binding level , when internal
 * Query Builder's instance is already created and language binding object 
 * should be instanciated.
 *
 * \param builder Midgard Query Builder object's instance
 *
 * \return type name
 *
 * Returned type name is a pointer and is owned by GLib system.
 * Caller shouldn't free it.
 */
const gchar *midgard_query_builder_get_type_name(MidgardQueryBuilder *builder);

/**
 * \ingroup qb
 *
 * Include deleted objects.
 *
 * \param builder , MidgardQueryBuilder instance pointer
 *
 * Normally Query Builder queries only undeleted objects. This method
 * forces QB instance to query both, deleted and undeleted objects.
 */
extern void midgard_query_builder_include_deleted(MidgardQueryBuilder *builder);

#endif
