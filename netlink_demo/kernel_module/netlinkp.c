#include <linux/string.h>
#include <linux/mm.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/sched.h>

#define ERR_MSG(msg) \
	printk(KERN_ERR "%s", msg)
#define MSG(msg) \
	printk(KERN_DEBUG "%s", msg)
#define SPEC_MSG(pid, msg) \
	printk(KERN_INFO "netlink:\tpid:\t%d\ndata:\t%s\n", pid, msg)
#define DEBUG_MSG(pid, msg) \
	printk(KERN_INFO "netlink DEBUG: pid:\t%d\tdata:\t%s\n", pid, msg)

#define VER_2_6_18
//#define VER_2_6_27
#define TASK_COMM_LEN 16
#define NETLINK_TEST 29
#define AUDIT_PATH "/home/audit/test"
#define MAX_LENGTH 256

static u32 pid = 0;
static struct sock *nl_sk = NULL;

int netlink_sendmsg(const void *buffer, unsigned int size)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(1200);

	if (!buffer || !nl_sk || !pid)
		return 1;

	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb)
	{
		ERR_MSG("netlink: allocate skb failed\n");
		return 1;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, 1200, 0);
	NETLINK_CB(skb).pid = 0;
	memcpy(NLMSG_DATA(nlh), buffer, size);

	if (netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT) < 0)
	{
		ERR_MSG("netlink: cannot unicast skb\n");
		return 1;
	}

	return 0;
}

/* run as pwd in shell */
void get_fullname(const char *pathname, char *fullname)
{
#if defined(VER_2_6_27)
	struct dentry *tmp_dentry = current->fs->pwd.dentry;
#elif defined(VER_2_6_18)
	struct dentry *tmp_dentry = current->fs->pwd;
#endif

	char tmp_path[MAX_LENGTH];
	char local_path[MAX_LENGTH];
	memset(tmp_path, 0, MAX_LENGTH);
	memset(local_path, 0, MAX_LENGTH);

	if (*pathname == '/')
	{
		strcpy(fullname, pathname);
		return;
	}

	while (tmp_dentry != NULL)
	{
		if (!strcmp(tmp_dentry->d_iname, "/"))
			break;
		strcpy(tmp_path, "/");
		strcat(tmp_path, tmp_dentry->d_iname);
		strcat(tmp_path, local_path);
		strcpy(local_path, tmp_path);
		tmp_dentry = tmp_dentry->d_parent;
	}
	strcpy(fullname, local_path);
	strcat(fullname, "/");
	strcat(fullname, pathname);

	return;
}

int audit_open(const char *pathname, int flags, int ret)
{
	char command_name[TASK_COMM_LEN];
	char fullname[MAX_LENGTH];
	unsigned int size;
	void *buffer;

	memset(fullname, 0, MAX_LENGTH);
	get_fullname(pathname, fullname);

	if (0 != strncmp(fullname, AUDIT_PATH, strlen(AUDIT_PATH)))
		return 1;

	strncpy(command_name, current->comm, TASK_COMM_LEN);
	size = strlen(fullname) + TASK_COMM_LEN + TASK_COMM_LEN + 1;
	buffer = kmalloc(size, 0);
	memset(buffer, 0, size);

	*((int *)buffer) = current->uid;
	*((int *)buffer + 1) = current->pid;
	*((int *)buffer + 2) = flags;
	*((int *)buffer + 3) = ret;

	strcpy((char *)((int *)buffer + 4), command_name);
	strcpy((char *)((int *)buffer + 4 + TASK_COMM_LEN / 4), fullname);

	MSG("try to send msg in netlink\n");
	netlink_sendmsg(buffer, size);

	return 0;
}

#ifdef VER_2_6_27
void nl_data_ready(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;

	skb = skb_get(__skb);
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = (struct nlmsghdr *)skb->data;
		if (0 != pid)
			MSG("pid is not zero");
		pid = nlh->nlmsg_pid;
		//prink("netlink:\npid:\t%d\ndata:\t%s\n", pid, (char *)NLMSG_DATA(nlh));
		//SPEC_MSG(pid, (char *)NLMSG_DATA(nlh));
		DEBUG_MSG(pid, (char *)NLMSG_DATA(nlh));
		
		kfree_skb(skb);
	}

	return;
}
#elif defined(VER_2_6_18)
void nl_data_ready(struct sock *sk, int len)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	skb = skb_dequeue(&(sk->sk_receive_queue));
	if (!skb)
	{
		ERR_MSG("no skb received\n");
		return;
	}
	nlh = (struct nlmsghdr *)skb->data;
	if (skb->len >= NLMSG_SPACE(0))
	{
		nlh = (struct nlmsghdr *)skb->data;
		if (0 != pid)
			MSG("pid is not zero");
		pid = nlh->nlmsg_pid;
		//SPEC_MSG(pid, (char *)NLMSG_DATA(nlh));
		DEBUG_MSG(pid, (char *)NLMSG_DATA(nlh));
		kfree_skb(skb);
	}

	return;
}
#endif

void netlink_init(void)
{
#if defined(VER_2_6_27)
	struct net netlink;
	nl_sk = netlink_kernel_create(&netlink, NETLINK_TEST, 0, nl_data_ready, NULL, THIS_MODULE);
#elif defined(VER_2_6_18)
	nl_sk = netlink_kernel_create(NETLINK_TEST, 0, nl_data_ready, THIS_MODULE);
#endif
	if (!nl_sk)
	{
		ERR_MSG("netlink: cannot create netlink socket\n");
	}
	else
	{
		MSG("netlink: create socket ok\n");
	}
}

void netlink_release(void)
{
	if (nl_sk)
	{
		sock_release(nl_sk->sk_socket);
		MSG("netlink: release socket\n");
	}
}
