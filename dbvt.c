#include "dbvt.h"
#include <float.h>

struct dbvt_t dbvt_Create()
{
    struct dbvt_t tree;
    tree.nodes = pool_CreateTyped(struct dbvt_node_t, 256);
    tree.root = NULL;
    return tree;
}

void dbvt_Destroy(struct dbvt_t *tree)
{
    if(tree != NULL && tree->nodes.buffer_count > 0)
    {
        pool_Destroy(&tree->nodes);
    }
}

struct dbvt_node_t *dbvt_AllocNode(struct dbvt_t *tree)
{
    struct dbvt_node_t *node = NULL;

    if(tree != NULL)
    {
        node = pool_AddElement(&tree->nodes, NULL);
        node->children[0] = NULL;
        node->children[1] = NULL;
        node->parent = NULL;
    }

    return node;
}

void dbvt_FreeNode(struct dbvt_t *tree, struct dbvt_node_t *node)
{
    if(tree != NULL && node != NULL && node->element_index != INVALID_POOL_INDEX)
    {
        pool_RemoveElement(&tree->nodes, node->element_index);
    }
}

uint32_t dbvt_BoxOverlap(vec3_t *max_a, vec3_t *min_a, vec3_t *max_b, vec3_t *min_b)
{
    return max_a->x >= min_b->x && min_a->x <= max_b->x &&
           max_a->y >= min_b->y && min_a->y <= max_b->y &&
           max_a->z >= min_b->z && min_a->z <= max_b->z;
}

float dbvt_NodesSurfaceArea(struct dbvt_node_t *node_a, struct dbvt_node_t *node_b)
{
    vec3_t max;
    vec3_t min;

    vec3_t_max(&max, &node_a->max, &node_b->max);
    vec3_t_min(&min, &node_a->min, &node_b->min);

    vec3_t extents;
    vec3_t_sub(&extents, &max, &min);

    // return extents.x * 2.0f + extents.y * 2.0f + extents.z * 2.0f;

    return (extents.x * extents.y + extents.x * extents.z + extents.y * extents.z) * 2.0f;
}

float dbvt_NodesVolumeDiff(struct dbvt_node_t *node_a, struct dbvt_node_t *node_b)
{
    vec3_t union_max;
    vec3_t union_min;

    vec3_t_max(&union_max, &node_a->max, &node_b->max);
    vec3_t_min(&union_min, &node_a->min, &node_b->min);

    vec3_t union_extents;
    vec3_t_sub(&union_extents, &union_max, &union_min);

    vec3_t extents;
    vec3_t_sub(&extents, &node_a->max, &node_a->min);

    return (union_extents.x * union_extents.y * union_extents.z) - (extents.x * extents.y * extents.z);
}

void dbvt_FindBestSibling_r(struct dbvt_t *tree, struct dbvt_node_t *node, struct dbvt_node_t *current_node, struct dbvt_node_t **best_node, float *best_area)
{
    if(current_node != node)
    {
        if(current_node->children[0] != current_node->children[1] && dbvt_BoxOverlap(&current_node->max, &current_node->min, &node->max, &node->min))
        {
            dbvt_FindBestSibling_r(tree, node, current_node->children[0], best_node, best_area);
            dbvt_FindBestSibling_r(tree, node, current_node->children[1], best_node, best_area);
        }

        float surface_area = dbvt_NodesSurfaceArea(current_node, node);

        if(surface_area < *best_area)
        {
            *best_area = surface_area;
            *best_node = current_node;
        }
    }
}

struct dbvt_node_t *dbvt_FindBestSibling(struct dbvt_t *tree, struct dbvt_node_t *node)
{
    float best_area = FLT_MAX;
    struct dbvt_node_t *best_node = NULL;
    dbvt_FindBestSibling_r(tree, node, tree->root, &best_node, &best_area);
    return best_node;
}

// struct dbvt_node_t *dbvt_FindBestSibling(struct dbvt_t *tree, struct dbvt_node_t *node)
// {
//     // float best_area = FLT_MAX;
//     struct dbvt_node_t *best_node = tree->root;

//     while(best_node->children[0] != best_node->children[1])
//     {
//         float area_a = dbvt_NodesSurfaceArea(best_node->children[0], node);
//         float area_b = dbvt_NodesSurfaceArea(best_node->children[1], node);
//         best_node = best_node->children[area_a > area_b];

//         // float diff_a = dbvt_NodesVolumeDiff(best_node->children[0], node);
//         // float diff_b = dbvt_NodesVolumeDiff(best_node->children[1], node);
//         // best_node = best_node->children[diff_a > diff_b];
//     }

//     return best_node;
// }

void dbvt_ComputeTreeExtents(struct dbvt_t *tree, struct dbvt_node_t *start_node)
{
    if(tree != NULL && start_node != NULL)
    {
        struct dbvt_node_t *node = start_node->parent;

        while(node != NULL)
        {
            vec3_t_max(&node->max, &node->children[0]->max, &node->children[1]->max);
            vec3_t_min(&node->min, &node->children[0]->min, &node->children[1]->min);
            node = node->parent;
        }
    }
}

void dbvt_InsertNode(struct dbvt_t *tree, struct dbvt_node_t *node)
{
    if(tree != NULL && node != NULL && node->element_index != INVALID_POOL_INDEX)
    {
        if(tree->root == NULL)
        {
            tree->root = node;
        }
        else if(node->parent == NULL && node != tree->root)
        {
            struct dbvt_node_t *sibling_node = dbvt_FindBestSibling(tree, node);
            struct dbvt_node_t *new_parent = dbvt_AllocNode(tree);

            if(sibling_node->parent == NULL)
            {
                tree->root = new_parent;
                // new_parent->depth = 0;
            }
            else
            {
                struct dbvt_node_t *parent = sibling_node->parent;
                uint32_t sibling_index = parent->children[1] == sibling_node;
                parent->children[sibling_index] = new_parent;
                // new_parent->depth = parent->depth + 1;
                new_parent->parent = parent;
            }

            new_parent->children[0] = sibling_node;
            new_parent->children[1] = node;

            sibling_node->parent = new_parent;
            node->parent = new_parent;

            dbvt_ComputeTreeExtents(tree, node);
        }
    }
}

void dbvt_RemoveNode(struct dbvt_t *tree, struct dbvt_node_t *node)
{
    if(tree != NULL && node != NULL)
    {
        if(node == tree->root)
        {
            tree->root = NULL;
        }
        else if(node->parent != NULL)
        {
            struct dbvt_node_t *parent_node = node->parent;
            uint32_t sibling_index = parent_node->children[0] == node;
            struct dbvt_node_t *sibling_node = parent_node->children[sibling_index];

            if(tree->root == parent_node)
            {
                tree->root = sibling_node;
                sibling_node->parent = NULL;
            }
            else
            {
                struct dbvt_node_t *grandpa_node = parent_node->parent;
                uint32_t parent_index = grandpa_node->children[1] == parent_node;
                grandpa_node->children[parent_index] = sibling_node;
                sibling_node->parent = grandpa_node;
            }

            dbvt_FreeNode(tree, parent_node);
            node->parent = NULL;
            dbvt_ComputeTreeExtents(tree, sibling_node);
        }
    }
}

void dbvt_UpdateNode(struct dbvt_t *tree, struct dbvt_node_t *node)
{
    if(tree != NULL && node != NULL)
    {
        if(node != tree->root)
        {
            if(node->parent != NULL)
            {
                dbvt_RemoveNode(tree, node);
            }

            dbvt_InsertNode(tree, node);
        }
    }
}

void dbvt_BoxOnDbvtContents_r(struct dbvt_t *tree, struct list_t *contents, vec3_t *box_min, vec3_t *box_max, struct dbvt_node_t *node)
{
    if(node != NULL)
    {
        if(vec3_t_box_overlap(box_min, box_max, &node->min, &node->max))
        {
            if(node->children[0] == node->children[1])
            {
                uint64_t index = list_AddElement(contents, NULL);
                void **content = list_GetElement(contents, index);
                *content = (void *)node->data;
            }
            else
            {
                dbvt_BoxOnDbvtContents_r(tree, contents, box_min, box_max, node->children[0]);
                dbvt_BoxOnDbvtContents_r(tree, contents, box_min, box_max, node->children[1]);
            }
        }
    }
}

void dbvt_BoxOnDbvtContents(struct dbvt_t *tree, struct list_t *contents, vec3_t *box_min, vec3_t *box_max)
{
    // contents->cursor = 0;

    if(tree != NULL && tree->root != NULL)
    {
        dbvt_BoxOnDbvtContents_r(tree, contents, box_min, box_max, tree->root);
    }
}

uint32_t dbvt_RayOnNode(vec3_t *ray_origin, vec3_t *ray_direction, struct dbvt_node_t *node, float *fract_min, float *fract_max)
{
    float local_fract_min = 0.0f;
    float local_fract_max = 1.0f;

    for(uint32_t axis_index = 0; axis_index < 3; axis_index++)
    {
        float dist0 = node->min.comps[axis_index] - ray_origin->comps[axis_index];
        float dist1 = node->max.comps[axis_index] - ray_origin->comps[axis_index];

        if(fabsf(ray_direction->comps[axis_index]) >= 0.0001f)
        {
            float fract0 = dist0 / ray_direction->comps[axis_index];
            float fract1 = dist1 / ray_direction->comps[axis_index];

            if(fract0 > fract1)
            {
                float temp = fract0;
                fract0 = fract1;
                fract1 = temp;
            }

            local_fract_min = fmaxf(local_fract_min, fract0);
            local_fract_max = fminf(local_fract_max, fract1);

            if(local_fract_max < local_fract_min)
            {
                return 0;
            }
        }
        else if(dist0 > 0.0f || dist1 < 0.0f)
        {
            /* ray is parallel and its origin is outside slab, so there's no intersection */
            return 0;
        }
    }

    *fract_min = local_fract_min;
    *fract_max = local_fract_max;

    return 1;
}

void dbvt_RayOnDbvtContents_r(struct dbvt_t *tree, struct list_t *hits, vec3_t *ray_origin, vec3_t *ray_direction, struct dbvt_node_t *node)
{
    float fracts[2][2] = {{0, 1}, {0, 1}};
    uint32_t hit_count = 0;

    for(uint32_t child_index = 0; child_index < 2; child_index++)
    {
        struct dbvt_node_t *child_node = node->children[child_index];

        if(!dbvt_RayOnNode(ray_origin, ray_direction, child_node, &fracts[child_index][0], &fracts[child_index][1]))
        {
            fracts[child_index][0] = FLT_MAX;
            continue;
        }

        hit_count++;
    }

    uint32_t closest_child_index = fracts[1][0] < fracts[0][0];

    for(uint32_t child_index = 0; child_index < hit_count; child_index++)
    {
        struct dbvt_node_t *child_node = node->children[closest_child_index];

        if(child_node->children[0] == child_node->children[1])
        {
            uint64_t hit_index = list_AddElement(hits, NULL);
            struct dbvt_ray_hit_t *ray_hit = list_GetElement(hits, hit_index);
            ray_hit->contents = (void *)child_node->data;
            ray_hit->fracts[0] = fracts[closest_child_index][0];
            ray_hit->fracts[1] = fracts[closest_child_index][1];
        }
        else
        {
            dbvt_RayOnDbvtContents_r(tree, hits, ray_origin, ray_direction, child_node);
        }

        closest_child_index ^= 1;
    }
}

uint32_t dbvt_RayOnDbvtContents(struct dbvt_t *tree, struct list_t *hits, vec3_t *ray_origin, vec3_t *ray_direction)
{
    uint32_t prev_count = hits->cursor;

    if(tree != NULL && tree->root != NULL)
    {
        struct dbvt_node_t *root = tree->root;
        float fract_min = 0.0f;
        float fract_max = 1.0f;

        if(dbvt_RayOnNode(ray_origin, ray_direction, root, &fract_min, &fract_max))
        {
            if(root->children[0] == root->children[1])
            {
                /* root is the only node in the tree */
                uint64_t hit_index = list_AddElement(hits, NULL);
                struct dbvt_ray_hit_t *ray_hit = list_GetElement(hits, hit_index);
                ray_hit->contents = (void *)root->data;
                ray_hit->fracts[0] = fract_min;
                ray_hit->fracts[1] = fract_max;
            }
            else
            {
                dbvt_RayOnDbvtContents_r(tree, hits, ray_origin, ray_direction, tree->root);
            }
        }
    }

    return hits->cursor != prev_count;
}