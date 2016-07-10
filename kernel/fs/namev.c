/******************************************************************************/
/* Important Spring 2016 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5f2e8d450c0c5851acd538befe33744efca0f1c4f9fb5f       */
/*         3c8feabc561a99e53d4d21951738da923cd1c7bbd11b30a1afb11172f80b       */
/*         984b1acfbbf8fae6ea57e0583d2610a618379293cb1de8e1e9d07e6287e8       */
/*         de7e82f3d48866aa2009b599e92c852f7dbf7a6e573f1c7228ca34b9f368       */
/*         faaef0c0fcf294cb                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        /* TODO: Kernel#2 lookup done
		NOT_YET_IMPLEMENTED("VFS: lookup done");
		*/
	KASSERT(NULL != dir);
	KASSERT(NULL != name);
	KASSERT(NULL != result);
	dbg(DBG_PRINT,"(GRADING2A 2.a)\n");

	dbg(DBG_PRINT,"(GRADING2B)\n");
	if(dir == NULL || dir->vn_ops->lookup == NULL){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		return -ENOTDIR;
	}
	dbg(DBG_PRINT,"(GRADING2B)\n");
	int retval = dir->vn_ops->lookup(dir, name, len, result);
	return retval;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
	/* TODO: Kernel#2 dir_namev done
    NOT_YET_IMPLEMENTED("VFS: dir_namev done");
    */
	KASSERT(NULL != pathname);
	KASSERT(NULL != namelen);
	KASSERT(NULL != name);
	KASSERT(NULL != res_vnode);
	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");

	dbg(DBG_PRINT,"(GRADING2B)\n");
	if(strlen(pathname) == 0){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		return -EINVAL;
	}

	/* set base */
	if(pathname[0] == '/'){
		base = vfs_root_vn;
		dbg(DBG_PRINT,"(GRADING2B)\n");
	}
	if(base == NULL){
		base = curproc->p_cwd;
		dbg(DBG_PRINT,"(GRADING2B)\n");
	}
	vnode_t *parent = base;
	*res_vnode = parent;
	char *start= (char*)pathname;
	char *end = start;
	char *nextStart = end;
	char *nextEnd = nextStart;
	int retval = 0;

	vref(parent);
	vref(*res_vnode);
	dbg(DBG_PRINT,"(GRADING2B)\n");
	while(*start == '/'){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		start ++;
	}
	if(*start == '\0'){/* path = "/////" */
		dbg(DBG_PRINT,"(GRADING2B)\n");
		vput(parent);
		*name = start;
		*namelen = 0;
		return 0;
	}
	end = start;
	dbg(DBG_PRINT,"(GRADING2B)\n");
	while(!(*end == '/' || *end == '\0')){
		end ++;
		dbg(DBG_PRINT,"(GRADING2B)\n");
	}
	if(end - start > NAME_LEN){
		vput(parent);
		vput(*res_vnode);
		dbg(DBG_PRINT,"(GRADING2B)\n");
		return -ENAMETOOLONG;
	}
	if(*end == '\0'){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		vput(parent);
		*name = start;
		*namelen = end - start;
		return 0;
	}
	while(1){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		nextStart = end;
		while(*nextStart == '/'){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			nextStart ++;
		}
		if(*nextStart == '\0'){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			vput(parent);
			*namelen = end - start;
			*name = start;
			return 0;
		}
		dbg(DBG_PRINT,"(GRADING2B)\n");
		nextEnd = nextStart;
		while(!(*nextEnd == '/' || *nextEnd == '\0')){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			nextEnd ++;
		}
/*		if(nextEnd - nextStart > NAME_LEN){
			dbg(DBG_PRINT,"(GRADING2) newpath\n");
			vput(parent);
			vput(*res_vnode);
			return -ENAMETOOLONG;
		}
*/		vput(parent);
		parent = *res_vnode;
		KASSERT(NULL != parent);
		dbg(DBG_PRINT,"(GRADING2A 2.b)\n");
		retval = lookup(parent, start, end -start, res_vnode);
		if(retval >= 0){
			dbg(DBG_PRINT,"(GRADING2B)\n");
			start = nextStart;
			end = nextEnd;
			continue;
		}else{
			dbg(DBG_PRINT,"(GRADING2B)\n");
			vput(parent);
			return retval;
		}
	}
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
		/* TODO: Kernel#2 open_namev done
        NOT_YET_IMPLEMENTED("VFS: open_namev done");
        */
	dbg(DBG_PRINT,"(GRADING2B)\n");
	size_t length;
	char *name;
	vnode_t *parentDir;
	int retval = dir_namev(pathname, &length, (const char**)&name, base, &parentDir);
	if(retval >= 0){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		retval = lookup(parentDir, name, length, res_vnode);
		if(retval >= 0){/* found file, open successfully. */
			vput(parentDir);
			dbg(DBG_PRINT,"(GRADING2B)\n");
			return retval;
		}else{
			dbg(DBG_PRINT,"(GRADING2B)\n");
			if((retval == -ENOENT)){/* create file */
				dbg(DBG_PRINT,"(GRADING2B)\n");
				if(!(flag & O_CREAT)){
					dbg(DBG_PRINT,"(GRADING2B)\n");
					vput(parentDir);
					return -ENOENT;
				}
				if((retval == -ENOENT) && (flag & O_CREAT)){
					KASSERT(NULL != parentDir->vn_ops->create);
					dbg(DBG_PRINT,"(GRADING2A 2.c)\n");
					dbg(DBG_PRINT,"(GRADING2B)\n");
				}
				dbg(DBG_PRINT,"(GRADING2B)\n");
				retval = parentDir->vn_ops->create(parentDir, name, length, res_vnode);
				vput(parentDir);
				return retval;
			}else{/* failed */
				dbg(DBG_PRINT,"(GRADING2B)\n");
				vput(parentDir);
				return retval;
			}
		}
	}else{
		dbg(DBG_PRINT,"(GRADING2B)\n");
		return retval;
	}
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
