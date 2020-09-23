#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "ftree.h"

//prototype for the generate_ftree function helper
struct TreeNode *recursive_ftree_builder(const char *fname, char *path);


/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {
    struct TreeNode *root_ptr = NULL;
    //Calls a helper function to build a FTree structure rooted at fname
    root_ptr = recursive_ftree_builder(fname, "");
    return root_ptr;
}


/*
 * Recursively build and return FTree rooted at fname
 *
 *
 */
struct TreeNode *recursive_ftree_builder(const char *fname, char *path){
  struct TreeNode *root_ptr = NULL;
  struct stat file_info;

  //Generate the string that contains the path to the current file and
  //stores it in the heap
  char *path_name = malloc(sizeof(char) * (strlen(fname) + strlen(path) + 2));
  strcpy(path_name, path);
  strcat(path_name, fname);

  //Assigning the stat struct to hold information of the current file
  if (lstat(path_name, &file_info) == -1){
     fprintf(stderr, "The path (%s) does not point to an existing entry!\n", path_name);
     return NULL;
  }

  //Dynamically Allocates memory for a TreeNode structure representing this current file
  root_ptr = malloc(sizeof(struct TreeNode));
  if (root_ptr == NULL){
    fprintf(stderr, "malloc for TreeNode failed");
    exit(1);
  }

  //Dynamically Allocates memory for a string representing the name of this file
  root_ptr -> fname = malloc(sizeof(char) * (strlen(fname) + 1));
  if (root_ptr -> fname == NULL){
    fprintf(stderr, "malloc for fname failed");
    exit(1);
  }

  //Below, the attributes of this TreeNode are initialized or assigned values to
  strcpy(root_ptr -> fname, fname);
  root_ptr -> permissions = file_info.st_mode & 0777;
  root_ptr -> next = NULL;
  root_ptr -> contents = NULL;

  if(S_ISREG(file_info.st_mode)){
    root_ptr -> type = '-';
  }
  if(S_ISLNK(file_info.st_mode)){
    root_ptr -> type = 'l';
  }
  if(S_ISDIR(file_info.st_mode)){
    //Modifies the path_name for possible recursive calls to build Ftree rooted
    //at this directory
    strcat(path_name, "/");

    root_ptr -> type = 'd';

    //Creates a pointer that points at this current directory
    DIR *d_ptr = opendir(path_name);
    if(d_ptr == NULL){
      fprintf(stderr, "Failed to open directory");
      exit(1);
    }

    //Creates a dirent struct to hold information of files this directory contains
    struct dirent *entry_ptr;

    //Assigns the dirent struct to hold information of the "first" file in
    //this directory (order decided by readdir)
    entry_ptr = readdir(d_ptr);

    short first_node_tracker = 1;

    //TreeNode pointer to hold a pointer to the first file inside the current
    //directory
    struct TreeNode *child_ptr = NULL;

    //while loop ensures that TreeNode structures for files on the same level
    //are generated and linked together in a linked list
    while (entry_ptr != NULL){
      //Conditional allows us to ignore files in this directory that begin with '.'
      if ((entry_ptr -> d_name)[0] != '.'){
        if (first_node_tracker == 1){
          //Recursively builds the TreeNode for the first file of this directory
          child_ptr = recursive_ftree_builder(entry_ptr -> d_name , path_name);
          (root_ptr -> contents) = child_ptr;
          first_node_tracker = 0;
        }
        else{
          //Recursively Builds the file adjacent to the current file within
          //the directory
          (child_ptr -> next) = recursive_ftree_builder(entry_ptr -> d_name , path_name);
          child_ptr = (child_ptr -> next);
        }
      }
      //Assigns entry_pointer to the information of the next file at the same level
      entry_ptr = readdir(d_ptr);
    }
    //closes the current directory
    if (closedir(d_ptr) == -1){
      fprintf(stderr, "Failed to properly close directory");
      exit(1);
    }
  }
  //deallocate the memory used by our current path_name string
  free(path_name);
  return root_ptr;
}


/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {

    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    while(root != NULL){
      //formats the spacing based on the current depth of the FTree
      printf("%*s", depth * 2, "");
      if (root -> type != 'd'){
        printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
      }
      else{
        printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
        if (root -> contents != NULL){
          //increment depth as a recursive call is about to take us one level
          //deeper into the FTree structure
          depth++;
          //Recursively print the FTree structure rooted at root -> contents
          print_ftree(root -> contents);
          //decrement depth since we have returned from recursive calls
          depth--;
        }
      }
      //prints the FTree structure adjacent to current root on the linked list
      root = root -> next;
    }
}


/*
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 *
 */
void deallocate_ftree (struct TreeNode *node) {
  if (node -> contents != NULL){
    //Recursively free all memory used by the FTree structure rooted
    //at the child of this current node
    deallocate_ftree(node -> contents);
  }
  if (node -> next != NULL){
    //Recursively free all memory used by the FTree structure rooted
    //at the adjacent node to this node on the linked list
    deallocate_ftree(node -> next);
  }
  free(node -> fname);
  //Memory allocated for this current node is freed
  free(node);
}
