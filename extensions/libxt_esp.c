/* Shared library add-on to iptables to add ESP support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <xtables.h>
#include <linux/netfilter/xt_esp.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"ESP v%s options:\n"
" --espspi [!] spi[:spi]\n"
"				match spi (range)\n",
IPTABLES_VERSION);
}

static struct option opts[] = {
	{ "espspi", 1, 0, '1' },
	{0}
};

static u_int32_t
parse_esp_spi(const char *spistr)
{
	unsigned long int spi;
	char* ep;

	spi =  strtoul(spistr,&ep,0) ;

	if ( spistr == ep ) {
		exit_error(PARAMETER_PROBLEM,
			   "ESP no valid digits in spi `%s'", spistr);
	}
	if ( spi == ULONG_MAX  && errno == ERANGE ) {
		exit_error(PARAMETER_PROBLEM,
			   "spi `%s' specified too big: would overflow", spistr);
	}	
	if ( *spistr != '\0'  && *ep != '\0' ) {
		exit_error(PARAMETER_PROBLEM,
			   "ESP error parsing spi `%s'", spistr);
	}
	return (u_int32_t) spi;
}

static void
parse_esp_spis(const char *spistring, u_int32_t *spis)
{
	char *buffer;
	char *cp;

	buffer = strdup(spistring);
	if ((cp = strchr(buffer, ':')) == NULL)
		spis[0] = spis[1] = parse_esp_spi(buffer);
	else {
		*cp = '\0';
		cp++;

		spis[0] = buffer[0] ? parse_esp_spi(buffer) : 0;
		spis[1] = cp[0] ? parse_esp_spi(cp) : 0xFFFFFFFF;
		if (spis[0] > spis[1])
			exit_error(PARAMETER_PROBLEM,
				   "Invalid ESP spi range: %s", spistring);
	}
	free(buffer);
}

/* Initialize the match. */
static void
init(struct xt_entry_match *m, unsigned int *nfcache)
{
	struct xt_esp *espinfo = (struct xt_esp *)m->data;

	espinfo->spis[1] = 0xFFFFFFFF;
}

#define ESP_SPI 0x01

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const void *entry,
      unsigned int *nfcache,
      struct xt_entry_match **match)
{
	struct xt_esp *espinfo = (struct xt_esp *)(*match)->data;

	switch (c) {
	case '1':
		if (*flags & ESP_SPI)
			exit_error(PARAMETER_PROBLEM,
				   "Only one `--espspi' allowed");
		check_inverse(optarg, &invert, &optind, 0);
		parse_esp_spis(argv[optind-1], espinfo->spis);
		if (invert)
			espinfo->invflags |= XT_ESP_INV_SPI;
		*flags |= ESP_SPI;
		break;
	default:
		return 0;
	}

	return 1;
}

/* Final check; we don't care. */
static void
final_check(unsigned int flags)
{
}

static void
print_spis(const char *name, u_int32_t min, u_int32_t max,
	    int invert)
{
	const char *inv = invert ? "!" : "";

	if (min != 0 || max != 0xFFFFFFFF || invert) {
		if (min == max)
			printf("%s:%s%u ", name, inv, min);
		else
			printf("%ss:%s%u:%u ", name, inv, min, max);
	}
}

/* Prints out the union ipt_matchinfo. */
static void
print(const void *ip,
      const struct xt_entry_match *match, int numeric)
{
	const struct xt_esp *esp = (struct xt_esp *)match->data;

	printf("esp ");
	print_spis("spi", esp->spis[0], esp->spis[1],
		    esp->invflags & XT_ESP_INV_SPI);
	if (esp->invflags & ~XT_ESP_INV_MASK)
		printf("Unknown invflags: 0x%X ",
		       esp->invflags & ~XT_ESP_INV_MASK);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_esp *espinfo = (struct xt_esp *)match->data;

	if (!(espinfo->spis[0] == 0
	    && espinfo->spis[1] == 0xFFFFFFFF)) {
		printf("--espspi %s", 
			(espinfo->invflags & XT_ESP_INV_SPI) ? "! " : "");
		if (espinfo->spis[0]
		    != espinfo->spis[1])
			printf("%u:%u ",
			       espinfo->spis[0],
			       espinfo->spis[1]);
		else
			printf("%u ",
			       espinfo->spis[0]);
	}

}

static struct xtables_match esp = { 
	.family		= AF_INET,
	.name 		= "esp",
	.version 	= IPTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct xt_esp)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_esp)),
	.help		= &help,
	.init		= &init,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

static struct xtables_match esp6 = { 
	.family		= AF_INET6,
	.name 		= "esp",
	.version 	= IPTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct xt_esp)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_esp)),
	.help		= &help,
	.init		= &init,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void
_init(void)
{
	xtables_register_match(&esp);
	xtables_register_match(&esp6);
}