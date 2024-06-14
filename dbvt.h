#ifndef DBVT_H
#define DBVT_H

#include "pool.h"
#include "list.h"
#include "maths.h"


/* forward declaration */
struct dbvt_node_t;

struct dbvt_link_t
{
    uint64_t                index;
    struct dbvt_link_t *    parent;
    struct dbvt_link_t *    children[2];
    struct dbvt_node_t *    node;
};

struct dbvt_node_t
{
    POOL_ELEMENT;
    struct dbvt_node_t *children[2];
    struct dbvt_node_t *parent;
    vec3_t              min;
    vec3_t              max;
    // uint64_t            entity;
    uintptr_t           data;
    // union
    // {
    //     void *          data;
        
    //     struct
    //     {
    //         uint32_t    depth;
    //         float       area;
    //     };
    // };  
};

struct dbvt_t
{
    struct pool_t           nodes;
    struct list_t           links;
    struct dbvt_node_t *    root;
};

struct dbvt_ray_hit_t
{
    void *  contents;
    float   fracts[2];
};

#ifdef __cplusplus
extern "C"
{
#endif

struct dbvt_t dbvt_Create();

void dbvt_Destroy(struct dbvt_t *tree);

struct dbvt_node_t *dbvt_AllocNode(struct dbvt_t *tree);

void dbvt_FreeNode(struct dbvt_t *tree, struct dbvt_node_t *node);

void dbvt_InsertNode(struct dbvt_t *tree, struct dbvt_node_t *node);

void dbvt_RemoveNode(struct dbvt_t *tree, struct dbvt_node_t *node);

void dbvt_UpdateNode(struct dbvt_t *tree, struct dbvt_node_t *node);

void dbvt_BoxOnDbvtContents(struct dbvt_t *tree, struct list_t *contents, vec3_t *box_min, vec3_t *box_max);

uint32_t dbvt_RayOnDbvtContents(struct dbvt_t *tree, struct list_t *contents, vec3_t *ray_origin, vec3_t *ray_direction);

#ifdef __cplusplus
}
#endif

#endif