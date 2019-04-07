#include "builtin.h"
#include "cache.h"
#include "commit-reach.h"
#include "object-store.h"
#include "object.h"
#include "parse-options.h"
#include "refs.h"
#include "string-list.h"
#include "tag.h"

#define PARENT1 (1u << 16)
#define PARENT2 (1u << 17)

static const char *const table_usage[] = {
	N_("git table [--heads] [--tags] [--remotes]"),
	NULL,
};

static int show_heads, show_tags, show_remotes;

static const struct option table_options[] = {
	OPT_BOOL(0, "heads", &show_heads, N_("show heads")),
	OPT_BOOL(0, "tags", &show_tags, N_("show tags")),
	OPT_BOOL(0, "remotes", &show_remotes, N_("show remotes")),
	OPT_END(),
};

static struct commit *head_commit = NULL;

struct pair {
	struct commit *a;
	char *a_name;
	struct commit *b;
	char *b_name;
	int ahead;
	int behind;
};

static struct commit *get_commit_reference(const char *arg)
{
	struct object_id revkey;
	struct commit *r;

	if (get_oid(arg, &revkey))
		die("Not a valid object name %s", arg);
	r = lookup_commit_reference(the_repository, &revkey);
	if (!r)
		die("Not a valid commit name %s", arg);

	return r;
}

static void print(const struct pair *p)
{
	if (p->ahead > 0 && p->behind > 0) {
		printf("[%s] is ahead by %d and behind by %d commit(s) from [%s]\n",
		       p->a_name, p->ahead, p->behind, p->b_name);
		return;
	}
	if (p->ahead > 0) {
		printf("[%s] is ahead by %d commit(s) from [%s]\n", p->a_name,
		       p->ahead, p->b_name);
		return;
	}
	if (p->behind > 0) {
		printf("[%s] is behind by %d commit(s) from [%s]\n", p->a_name,
		       p->behind, p->b_name);
		return;
	}
}

static int show_ref(const char *refname, const struct object_id *oid, int flag,
		    void *cbdata)
{
	int match = 0;

	match |= show_heads && starts_with(refname, "refs/heads/");
	match |= show_tags && starts_with(refname, "refs/tags/");
	match |= show_remotes && starts_with(refname, "refs/remotes/");

	if (!match)
		return 0;

	struct commit *commit = lookup_commit_or_die(oid, refname);
	if (!commit)
		return 0;

	if (commit == head_commit)
		return 0;

	struct commit_list *mb =
		get_merge_bases_many_dirty(head_commit, 1, &commit);
	int ahead = count_marked_commits(head_commit, PARENT1);
	int behind = count_marked_commits_many(1, &commit, PARENT2);
	free_commit_list(mb);

	struct pair *p = xmalloc(sizeof(struct pair));
	p->a = head_commit;
	p->a_name = "HEAD";
	p->b = commit;
	p->b_name = strdup(refname);
	p->ahead = ahead;
	p->behind = behind;

	print(p);

	return 0;
}

int cmd_table(int argc, const char **argv, const char *prefix)
{
	argc = parse_options(argc, argv, prefix, table_options, table_usage, 0);

	if (!show_heads && !show_tags)
		show_heads = 1;

	head_commit = get_commit_reference("HEAD");

	for_each_ref(show_ref, NULL);

	return 0;
}
