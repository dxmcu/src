/*	$NetBSD: tda.c,v 1.4 2011/04/03 06:25:11 jdc Exp $	*/
/*	$OpenBSD: tda.c,v 1.4 2008/02/27 17:25:00 robert Exp $ */

/*
 * Copyright (c) 2008 Robert Nagy <robert@openbsd.org>
 * Copyright (c) 2008 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/i2c/i2cvar.h>

/* fan control registers */
#define TDA_SYSFAN_REG		0xf0
#define TDA_CPUFAN_REG		0xf2
#define TDA_PSFAN_REG		0xf4

#define TDA_FANSPEED_MIN        0x0c
#define TDA_FANSPEED_MAX        0x3f

#define TDA_PSFAN_ON            0x1f
#define TDA_PSFAN_OFF           0x00

/* Internal and External temperature senor numbers */
#define SENSOR_TEMP_EXT		0
#define SENSOR_TEMP_INT		1

#define CPU_TEMP_MAX		(67 * 1000000 + 273150000)
#define CPU_TEMP_MIN		(57 * 1000000 + 273150000)
#define SYS_TEMP_MAX		(30 * 1000000 + 273150000)
#define SYS_TEMP_MIN		(20 * 1000000 + 273150000)

struct tda_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_tag;
	i2c_addr_t		sc_addr;

	u_int16_t		sc_cfan_speed;	/* current CPU fan speed */
	u_int16_t		sc_sfan_speed;	/* current SYS fan speed */

	callout_t		sc_timer;
};

int	tda_match(struct device *, struct cfdata *, void *);
void	tda_attach(struct device *, struct device *, void *);

void	tda_setspeed(struct tda_softc *);
static void	tda_adjust(void *);
static void	tda_timeout(void *);


CFATTACH_DECL_NEW(tda, sizeof(struct tda_softc),
	tda_match, tda_attach, NULL, NULL);

int
tda_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	char name[32];

	/* Only attach on the Sun Blade 1000/2000. */
	if (OF_getprop(findroot(), "name", name, sizeof(name)) <= 0)
		return (0);
	if (strcmp(name, "SUNW,Sun-Blade-1000") != 0)
		return (0);

	/*
	 * No need for "compatible" matching, we know exactly what
	 * firmware calls us.
	 */
	if (ia->ia_name == NULL)
		return(0);
	return strcmp(ia->ia_name, "fan-control") == 0;
}

void
tda_attach(struct device *parent, struct device *self, void *aux)
{
	struct tda_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_normal(": %s\n", ia->ia_name);
	aprint_naive(": Environment sensor\n");

	/*
	 * Set the fans to maximum speed and save the power levels;
	 * the controller is write-only.
	 */
	sc->sc_cfan_speed = sc->sc_sfan_speed = (TDA_FANSPEED_MAX+TDA_FANSPEED_MIN)/2;
	tda_setspeed(sc);

	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
	callout_reset(&sc->sc_timer, hz*20, tda_timeout, sc);
}

static void
tda_timeout(void *v)
{
	struct tda_softc *sc = v;

	sysmon_task_queue_sched(0, tda_adjust, sc);
	callout_reset(&sc->sc_timer, hz*60, tda_timeout, sc);
}

void
tda_setspeed(struct tda_softc *sc)
{
	u_int8_t cmd[2];

	if (sc->sc_cfan_speed < TDA_FANSPEED_MIN)
		sc->sc_cfan_speed = TDA_FANSPEED_MIN;
	if (sc->sc_sfan_speed < TDA_FANSPEED_MIN)
		sc->sc_sfan_speed = TDA_FANSPEED_MIN;
	if (sc->sc_cfan_speed > TDA_FANSPEED_MAX)
		sc->sc_cfan_speed = TDA_FANSPEED_MAX;
	if (sc->sc_sfan_speed > TDA_FANSPEED_MAX)
		sc->sc_sfan_speed = TDA_FANSPEED_MAX;

	iic_acquire_bus(sc->sc_tag, 0);

	cmd[0] = TDA_CPUFAN_REG;
	cmd[1] = sc->sc_cfan_speed;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof(cmd), NULL, 0, 0)) {
		aprint_error_dev(sc->sc_dev, "cannot write cpu-fan register\n");
		iic_release_bus(sc->sc_tag, 0);
		return;
        }

	cmd[0] = TDA_SYSFAN_REG;
	cmd[1] = sc->sc_sfan_speed;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof(cmd), NULL, 0, 0)) {
		aprint_error_dev(sc->sc_dev, "cannot write system-fan register\n");
		iic_release_bus(sc->sc_tag, 0);
		return;
        }

	iic_release_bus(sc->sc_tag, 0);

	aprint_debug_dev(sc->sc_dev, "changed fan speed to cpu=%d system=%d\n",
		sc->sc_cfan_speed, sc->sc_sfan_speed); 
}

static bool
is_cpu_sensor(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	return strcmp(edata->desc, "external") == 0;
}

static bool
is_system_sensor(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	return strcmp(edata->desc, "internal") == 0;
}

static void
tda_adjust(void *v)
{
	struct tda_softc *sc = v;
	u_int64_t ctemp, stemp;
	u_int16_t cspeed, sspeed;

	/* Default to running the fans at maximum speed. */
	sspeed = cspeed = TDA_FANSPEED_MAX;

	/* fetch maximum current temperature */
	ctemp = sysmon_envsys_get_max_value(is_cpu_sensor, true);
	stemp = sysmon_envsys_get_max_value(is_system_sensor, true);

	/* the predicates for selecting sensors must have gone wrong */
	if (ctemp == 0 || stemp == 0) {
		aprint_error_dev(sc->sc_dev, "skipping temp adjustment"
			" - no sensor values\n");
		return;
	}

	aprint_debug_dev(sc->sc_dev, "current temperature: cpu %" PRIu64
		" system %" PRIu64 "\n",
		ctemp, stemp);

	if (ctemp < CPU_TEMP_MIN)
		cspeed = TDA_FANSPEED_MIN;
	else if (ctemp < CPU_TEMP_MAX)
		cspeed = TDA_FANSPEED_MIN +
			(ctemp - CPU_TEMP_MIN) * 
			(TDA_FANSPEED_MAX - TDA_FANSPEED_MIN) / 
			(CPU_TEMP_MAX - CPU_TEMP_MIN);

	if (stemp < SYS_TEMP_MIN)
		sspeed = TDA_FANSPEED_MIN;
	else if (stemp < SYS_TEMP_MAX)
		sc->sc_sfan_speed = TDA_FANSPEED_MIN +
			(stemp - SYS_TEMP_MIN) * 
			(TDA_FANSPEED_MAX - TDA_FANSPEED_MIN) / 
			(SYS_TEMP_MAX - SYS_TEMP_MIN);

	if (sspeed == sc->sc_sfan_speed && cspeed == sc->sc_cfan_speed)
		return;

	sc->sc_sfan_speed = sspeed;
	sc->sc_cfan_speed = cspeed;
	tda_setspeed(sc);
}
