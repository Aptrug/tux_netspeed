#include <linux/if.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <unistd.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define WARNIF(x)                                                              \
	do {                                                                   \
		if (x) {                                                       \
			perror(__FILE__ ":" TOSTRING(__LINE__) ": " #x);       \
		}                                                              \
	} while (0)

static void
get_rx_and_tx_bytes(struct nlmsghdr *const h, unsigned int *const rx_bytes,
 unsigned int *const tx_bytes)
{

	struct rtattr *attr_ptr;
	long attr_len;
	const struct rtnl_link_stats *netstats;
	struct ifinfomsg *ifi_ptr;

	ifi_ptr = NLMSG_DATA(h);

	if (!(ifi_ptr->ifi_flags & IFF_RUNNING) ||
	    ifi_ptr->ifi_flags & IFF_NOARP) {
		return;
	}

	attr_len = NLMSG_PAYLOAD(h, sizeof(struct ifinfomsg));

	for (attr_ptr = IFLA_RTA(ifi_ptr); RTA_OK(attr_ptr, attr_len);
	     attr_ptr = RTA_NEXT(attr_ptr, attr_len)) {
		switch (attr_ptr->rta_type) {
		case IFLA_STATS:
			netstats = (struct rtnl_link_stats *)RTA_DATA(attr_ptr);
			*rx_bytes += netstats->rx_bytes;
			*tx_bytes += netstats->tx_bytes;
			break;
		default:
			break;
		}
	}
}

static unsigned char
iterate_over_interfaces(struct nlmsghdr *const buff, long len,
 unsigned int *const rx_bytes, unsigned int *const tx_bytes)
{
	struct nlmsghdr *msg_ptr = (struct nlmsghdr *)buff;
	for (; NLMSG_OK(msg_ptr, (unsigned int)len);
	     msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
		switch (msg_ptr->nlmsg_type) {
		case NLMSG_DONE:
			return 0;
		default:
			get_rx_and_tx_bytes(msg_ptr, rx_bytes, tx_bytes);
			break;
		}
	}
	return 1;
}

#define BUFSIZE 8192

static void
get_netspeed(unsigned int *const rx_bytes, unsigned int *const tx_bytes)
{
	struct {
		struct nlmsghdr hdr;
		struct rtgenmsg gen;
		const char pad[3];
	} req = {0};
	int sockfd;
	struct iovec iov;
	struct msghdr msg = {0};
	struct nlmsghdr buff[BUFSIZE];
	long len;

	req.hdr.nlmsg_len = sizeof(req);
	req.hdr.nlmsg_type = RTM_GETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.gen.rtgen_family = AF_INET;

	iov.iov_base = &req;
	iov.iov_len = req.hdr.nlmsg_len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	WARNIF(sockfd == -1);

	WARNIF(sendmsg(sockfd, &msg, 0) == -1);

	msg.msg_iov->iov_base = buff;
	msg.msg_iov->iov_len = BUFSIZE;

	*rx_bytes = 0;
	*tx_bytes = 0;
	for (;;) {
		len = recvmsg(sockfd, &msg, 0);
		WARNIF(len == -1);
		if (!iterate_over_interfaces(buff, len, rx_bytes, tx_bytes)) {
			break;
		}
	}
	WARNIF(close(sockfd) == -1);
}

struct readable_netspeed {
	double value;
	const char *unit;
};

static struct readable_netspeed
calculate_netspeed(const unsigned int bytes)
{
	struct readable_netspeed netspeed;
	if (bytes > 1048576) {
		netspeed.value = (double)bytes / 1048576.0;
		netspeed.unit = "MiB/s";
	} else if (bytes > 1024) {
		netspeed.value = (double)bytes / 1024.0;
		netspeed.unit = "KiB/s";
	} else {
		netspeed.value = (double)bytes;
		netspeed.unit = "B/s";
	}
	return netspeed;
}

int
main()
{
	unsigned int rx_bytes;
	unsigned int tx_bytes;
	unsigned int previous_rx_bytes;
	unsigned int previous_tx_bytes;
	struct readable_netspeed download_rate;
	struct readable_netspeed upload_rate;

	const char *const printf_format =
	 isatty(0) ? "\r\033[0KD:%.1f %s | U:%.1f %s" : "D:%.1f %s | U:%.1f %s\n";

	get_netspeed(&rx_bytes, &tx_bytes);
	previous_rx_bytes = rx_bytes;
	previous_tx_bytes = tx_bytes;

	for (;;) {
		sleep(1);
		get_netspeed(&rx_bytes, &tx_bytes);

		download_rate =
		 calculate_netspeed(rx_bytes - previous_rx_bytes);
		upload_rate = calculate_netspeed(tx_bytes - previous_tx_bytes);
		printf(printf_format, download_rate.value, download_rate.unit,
		 upload_rate.value, upload_rate.unit);
		fflush(stdout);

		previous_rx_bytes = rx_bytes;
		previous_tx_bytes = tx_bytes;
	}
}
