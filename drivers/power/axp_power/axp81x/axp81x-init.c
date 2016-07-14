/*
 * Battery charger driver for allwinnertech AXP81X
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/reboot.h>
#include "../axp-cfg.h"
#include "axp81x-sply.h"

void axp81x_power_off(int power_start)
{
	uint8_t val;
	struct axp_dev *axp;
	axp = axp_dev_lookup(AXP81X);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return;
	}
	if(axp81x_config.pmu_pwroff_vol >= 2600 && axp81x_config.pmu_pwroff_vol <= 3300){
		if (axp81x_config.pmu_pwroff_vol > 3200){
			val = 0x7;
		}else if (axp81x_config.pmu_pwroff_vol > 3100){
			val = 0x6;
		}else if (axp81x_config.pmu_pwroff_vol > 3000){
			val = 0x5;
		}else if (axp81x_config.pmu_pwroff_vol > 2900){
			val = 0x4;
		}else if (axp81x_config.pmu_pwroff_vol > 2800){
			val = 0x3;
		}else if (axp81x_config.pmu_pwroff_vol > 2700){
			val = 0x2;
		}else if (axp81x_config.pmu_pwroff_vol > 2600){
			val = 0x1;
		}else
			val = 0x0;
		axp_update(axp->dev, AXP81X_VOFF_SET, val, 0x7);
	}
	val = 0xff;
	printk("[axp] send power-off command!\n");

	mdelay(20);

	if(power_start != 1){
		axp_read(axp->dev, AXP81X_STATUS, &val);
		if(val & 0xF0){
			axp_read(axp->dev, AXP81X_MODE_CHGSTATUS, &val);
			if(val & 0x20) {
				printk("[axp] set flag!\n");
				axp_read(axp->dev, AXP81X_BUFFERC, &val);
				if (0x0d != val)
					axp_write(axp->dev, AXP81X_BUFFERC, 0x0f);
				mdelay(20);
				printk("[axp] reboot!\n");
				machine_restart(NULL);
				printk("[axp] warning!!! arch can't ,reboot, maybe some error happend!\n");
			}
		}
	}
	axp_read(axp->dev, AXP81X_BUFFERC, &val);
	if (0x0d != val)
		axp_write(axp->dev, AXP81X_BUFFERC, 0x00);

	mdelay(20);
	axp_set_bits(axp->dev, AXP81X_OFF_CTL, 0x80);
	mdelay(20);
	printk("[axp] warning!!! axp can't power-off, maybe some error happend!\n");
}

#if defined  (CONFIG_AXP_CHARGEINIT)
static void axp_set_charge(struct axp_charger *charger)
{
	uint8_t val=0x00;
	uint8_t tmp=0x00;

	if(charger->chgvol < 4150000){
		val &= ~(3 << 5);
	}else if (charger->chgvol<4200000){
		val &= ~(3 << 5);
		val |= 1 << 5;
	}else if (charger->chgvol<4350000){
		val &= ~(3 << 5);
		val |= 1 << 6;
	}else
		val |= 3 << 5;

	spin_lock(&charger->charger_lock);
	if(charger->chgcur == 0)
		charger->chgen = 0;

	if(charger->chgcur< 300000)
		charger->chgcur = 300000;
	else if(charger->chgcur > 2550000)
		charger->chgcur = 2550000;
	spin_unlock(&charger->charger_lock);

	val |= (charger->chgcur - 300000) / 150000 ;
	if(charger ->chgend == 10)
		val &= ~(1 << 4);
	else
		val |= 1 << 4;

	val &= 0x7F;
	val |= charger->chgen << 7;
	spin_lock(&charger->charger_lock);
	if(charger->chgpretime < 30)
		charger->chgpretime = 30;
	if(charger->chgcsttime < 360)
		charger->chgcsttime = 360;
	spin_unlock(&charger->charger_lock);

	tmp = ((((charger->chgpretime - 40) / 10) << 6)  \
	| ((charger->chgcsttime - 360) / 120));
	axp_write(charger->master, AXP81X_CHARGE_CONTROL1,val);
	axp_update(charger->master, AXP81X_CHARGE_CONTROL2,tmp,0xC2);
}
#else
static void axp_set_charge(struct axp_charger *charger)
{

}
#endif

#if defined  (CONFIG_AXP_CHARGEINIT)
static int axp_battery_adc_set(struct axp_charger *charger)
{
	int ret ;
	uint8_t val;

	/*enable adc and set adc */
	val= AXP81X_ADC_BATVOL_ENABLE | AXP81X_ADC_BATCUR_ENABLE;
	if (0 != axp81x_config.pmu_temp_enable)
		val = val | AXP81X_ADC_TSVOL_ENABLE;

	ret = axp_update(charger->master, AXP81X_ADC_CONTROL, val , AXP81X_ADC_BATVOL_ENABLE | AXP81X_ADC_BATCUR_ENABLE | AXP81X_ADC_TSVOL_ENABLE);
	if (ret)
		return ret;
	ret = axp_read(charger->master, AXP81X_ADC_CONTROL3, &val);
	switch (charger->sample_time/100){
		case 1: val &= ~(3 << 6);break;
		case 2: val &= ~(3 << 6);val |= 1 << 6;break;
		case 4: val &= ~(3 << 6);val |= 2 << 6;break;
		case 8: val |= 3 << 6;break;
	default: break;
	}
	if (0 != axp81x_config.pmu_temp_enable)
		val &= (~(1 << 2));
	ret = axp_write(charger->master, AXP81X_ADC_CONTROL3, val);
	if (ret)
		return ret;
	return 0;
}
#else
static int axp_battery_adc_set(struct axp_charger *charger)
{
  return 0;
}
#endif

static int axp_battery_first_init(struct axp_charger *charger)
{
	int ret;
	uint8_t val;

	axp_set_charge(charger);
	ret = axp_battery_adc_set(charger);
	if(ret)
	return ret;
	ret = axp_read(charger->master, AXP81X_ADC_CONTROL3, &val);

	spin_lock(&charger->charger_lock);
	switch ((val >> 6) & 0x03){
		case 0: charger->sample_time = 100;break;
		case 1: charger->sample_time = 200;break;
		case 2: charger->sample_time = 400;break;
		case 3: charger->sample_time = 800;break;
		default:break;
	}
	spin_unlock(&charger->charger_lock);

	return ret;
}


int axp81x_init(struct axp_charger *charger)
{
	int ret = 0, var = 0;
	uint8_t tmp = 0, val = 0;
	uint8_t ocv_cap[63];
	int Cur_CoulombCounter,rdc;

	ret = axp_battery_first_init(charger);
	if (ret)
		goto err_charger_init;

	/* usb voltage limit */
	if((axp81x_config.pmu_usbvol) && (axp81x_config.pmu_usbvol_limit)){
		axp_set_bits(charger->master, AXP81X_CHARGE_VBUS, 0x40);
		var = axp81x_config.pmu_usbvol * 1000;
		if(var >= 4000000 && var <=4700000){
			tmp = (var - 4000000)/100000;
			axp_read(charger->master, AXP81X_CHARGE_VBUS,&val);
			val &= 0xC7;
			val |= tmp << 3;
			axp_write(charger->master, AXP81X_CHARGE_VBUS,val);
		}
	}else
		axp_clr_bits(charger->master, AXP81X_CHARGE_VBUS, 0x40);

#if defined (CONFIG_AXP_CHGCHANGE)
	if(axp81x_config.pmu_runtime_chgcur == 0)
		axp_clr_bits(charger->master,AXP81X_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(charger->master,AXP81X_CHARGE_CONTROL1,0x80);
	printk("pmu_runtime_chgcur = %d\n", axp81x_config.pmu_runtime_chgcur);
	if(axp81x_config.pmu_runtime_chgcur >= 300000 && axp81x_config.pmu_runtime_chgcur <= 2550000){
		tmp = (axp81x_config.pmu_runtime_chgcur -200001)/150000;
		charger->chgcur = tmp *150000 + 300000;
		axp_update(charger->master, AXP81X_CHARGE_CONTROL1, tmp,0x0F);
	}else if(axp81x_config.pmu_runtime_chgcur < 300000){
		axp_clr_bits(axp_charger->master, AXP81X_CHARGE_CONTROL1,0x0F);
	}else{
		axp_set_bits(axp_charger->master, AXP81X_CHARGE_CONTROL1,0x0F);
	}
#endif

	/* set lowe power warning/shutdown level */
	axp_write(charger->master, AXP81X_WARNING_LEVEL,((axp81x_config.pmu_battery_warning_level1-5) << 4)+axp81x_config.pmu_battery_warning_level2);
	ocv_cap[0]  = axp81x_config.pmu_bat_para1;
	ocv_cap[1]  = 0xC1;
	ocv_cap[2]  = axp81x_config.pmu_bat_para2;
	ocv_cap[3]  = 0xC2;
	ocv_cap[4]  = axp81x_config.pmu_bat_para3;
	ocv_cap[5]  = 0xC3;
	ocv_cap[6]  = axp81x_config.pmu_bat_para4;
	ocv_cap[7]  = 0xC4;
	ocv_cap[8]  = axp81x_config.pmu_bat_para5;
	ocv_cap[9]  = 0xC5;
	ocv_cap[10] = axp81x_config.pmu_bat_para6;
	ocv_cap[11] = 0xC6;
	ocv_cap[12] = axp81x_config.pmu_bat_para7;
	ocv_cap[13] = 0xC7;
	ocv_cap[14] = axp81x_config.pmu_bat_para8;
	ocv_cap[15] = 0xC8;
	ocv_cap[16] = axp81x_config.pmu_bat_para9;
	ocv_cap[17] = 0xC9;
	ocv_cap[18] = axp81x_config.pmu_bat_para10;
	ocv_cap[19] = 0xCA;
	ocv_cap[20] = axp81x_config.pmu_bat_para11;
	ocv_cap[21] = 0xCB;
	ocv_cap[22] = axp81x_config.pmu_bat_para12;
	ocv_cap[23] = 0xCC;
	ocv_cap[24] = axp81x_config.pmu_bat_para13;
	ocv_cap[25] = 0xCD;
	ocv_cap[26] = axp81x_config.pmu_bat_para14;
	ocv_cap[27] = 0xCE;
	ocv_cap[28] = axp81x_config.pmu_bat_para15;
	ocv_cap[29] = 0xCF;
	ocv_cap[30] = axp81x_config.pmu_bat_para16;
	ocv_cap[31] = 0xD0;
	ocv_cap[32] = axp81x_config.pmu_bat_para17;
	ocv_cap[33] = 0xD1;
	ocv_cap[34] = axp81x_config.pmu_bat_para18;
	ocv_cap[35] = 0xD2;
	ocv_cap[36] = axp81x_config.pmu_bat_para19;
	ocv_cap[37] = 0xD3;
	ocv_cap[38] = axp81x_config.pmu_bat_para20;
	ocv_cap[39] = 0xD4;
	ocv_cap[40] = axp81x_config.pmu_bat_para21;
	ocv_cap[41] = 0xD5;
	ocv_cap[42] = axp81x_config.pmu_bat_para22;
	ocv_cap[43] = 0xD6;
	ocv_cap[44] = axp81x_config.pmu_bat_para23;
	ocv_cap[45] = 0xD7;
	ocv_cap[46] = axp81x_config.pmu_bat_para24;
	ocv_cap[47] = 0xD8;
	ocv_cap[48] = axp81x_config.pmu_bat_para25;
	ocv_cap[49] = 0xD9;
	ocv_cap[50] = axp81x_config.pmu_bat_para26;
	ocv_cap[51] = 0xDA;
	ocv_cap[52] = axp81x_config.pmu_bat_para27;
	ocv_cap[53] = 0xDB;
	ocv_cap[54] = axp81x_config.pmu_bat_para28;
	ocv_cap[55] = 0xDC;
	ocv_cap[56] = axp81x_config.pmu_bat_para29;
	ocv_cap[57] = 0xDD;
	ocv_cap[58] = axp81x_config.pmu_bat_para30;
	ocv_cap[59] = 0xDE;
	ocv_cap[60] = axp81x_config.pmu_bat_para31;
	ocv_cap[61] = 0xDF;
	ocv_cap[62] = axp81x_config.pmu_bat_para32;
	axp_writes(charger->master, 0xC0,63,ocv_cap);
	/* pok open time set */
	axp_read(charger->master,AXP81X_POK_SET,&val);
	if(axp81x_config.pmu_pekon_time < 1000)
		val &= 0x3f;
	else if(axp81x_config.pmu_pekon_time < 2000){
		val &= 0x3f;
		val |= 0x40;
	}else if(axp81x_config.pmu_pekon_time < 3000){
		val &= 0x3f;
		val |= 0x80;
	}else {
		val &= 0x3f;
		val |= 0xc0;
	}
	axp_write(charger->master,AXP81X_POK_SET,val);
	/* pok long time set*/
	if(axp81x_config.pmu_peklong_time < 1000)
		var = 1000;
	if(axp81x_config.pmu_peklong_time > 2500)
		var = 2500;
	axp_read(charger->master,AXP81X_POK_SET,&val);
	val &= 0xcf;
	val |= (((var - 1000) / 500) << 4);
	axp_write(charger->master,AXP81X_POK_SET,val);
	/* pek offlevel poweroff en set*/
	if(axp81x_config.pmu_pekoff_en)
		var = 1;
	else
		var = 0;
	axp_read(charger->master,AXP81X_POK_SET,&val);
	val &= 0xf7;
	val |= (var << 3);
	axp_write(charger->master,AXP81X_POK_SET,val);
	/*Init offlevel restart or not */
	if(axp81x_config.pmu_pekoff_func)
		axp_set_bits(charger->master,AXP81X_POK_SET,0x04); //restart
	else
		axp_clr_bits(charger->master,AXP81X_POK_SET,0x04); //not restart

	if(10 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x00;
	else if(20 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x01;
	else if(30 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x02;
	else if(40 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x03;
	else if(50 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x04;
	else if(60 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x05;
	else if(70 > axp81x_config.pmu_pekoff_delay_time)
		val = 0x06;
	else
		val = 0x07;
	axp_write(charger->master,AXP81X_POK_DELAY_SET,val);

	/* pek delay set */
	axp_read(charger->master,AXP81X_OFF_CTL,&val);
	val &= 0xfc;
	if (axp81x_config.pmu_pwrok_time < 32)
		val |= ((axp81x_config.pmu_pwrok_time / 8) - 1);
	else
		val |= ((axp81x_config.pmu_pwrok_time / 32) + 1);
	axp_write(charger->master,AXP81X_OFF_CTL,val);

	axp_read(charger->master,AXP81X_DCDC_MONITOR,&val);
	if(axp81x_config.pmu_pwrok_shutdown_en)
		val |= 0x40;
	axp_write(charger->master,AXP81X_DCDC_MONITOR,val);

	/* pek offlevel time set */
	if(axp81x_config.pmu_pekoff_time < 4000)
		var = 4000;
	if(axp81x_config.pmu_pekoff_time > 10000)
		var =10000;
	var = (var - 4000) / 2000 ;
	axp_read(charger->master,AXP81X_POK_SET,&val);
	val &= 0xfc;
	val |= var ;
	axp_write(charger->master,AXP81X_POK_SET,val);
	/*Init 16's Reset PMU en */
	if(axp81x_config.pmu_reset)
		axp_set_bits(charger->master,0x8F,0x08); //enable
	else
		axp_clr_bits(charger->master,0x8F,0x08); //disable
	/*Init IRQ wakeup en*/
	if(axp81x_config.pmu_IRQ_wakeup)
		axp_set_bits(charger->master,0x8F,0x80); //enable
	else
		axp_clr_bits(charger->master,0x8F,0x80); //disable
	/*Init N_VBUSEN status*/
	if(axp81x_config.pmu_vbusen_func)
		axp_set_bits(charger->master,0x8F,0x10); //output
	else
		axp_clr_bits(charger->master,0x8F,0x10); //input
	/*Init InShort status*/
	if(axp81x_config.pmu_inshort)
		axp_set_bits(charger->master,0x8F,0x60); //InShort
	else
		axp_clr_bits(charger->master,0x8F,0x60); //auto detect
	/*Init CHGLED function*/
	if(axp81x_config.pmu_chgled_func)
		axp_set_bits(charger->master,0x32,0x08); //control by charger
	else
		axp_clr_bits(charger->master,0x32,0x08); //drive MOTO
	/*set CHGLED Indication Type*/
	if(axp81x_config.pmu_chgled_type)
		axp_set_bits(charger->master,0x34,0x10); //Type B
	else
		axp_clr_bits(charger->master,0x34,0x10); //Type A
	/*Init PMU Over Temperature protection*/
	if(axp81x_config.pmu_hot_shutdowm)
		axp_set_bits(charger->master,0x8f,0x04); //enable
	else
		axp_clr_bits(charger->master,0x8f,0x04); //disable
	/*Init battery capacity correct function*/
	if(axp81x_config.pmu_batt_cap_correct)
		axp_set_bits(charger->master,0xb8,0x20); //enable
	else
		axp_clr_bits(charger->master,0xb8,0x20); //disable
	/* Init battery regulator enable or not when charge finish*/
	if(axp81x_config.pmu_bat_regu_en)
		axp_set_bits(charger->master,0x34,0x20); //enable
	else
		axp_clr_bits(charger->master,0x34,0x20); //disable
	if(!axp81x_config.pmu_batdeten)
		axp_clr_bits(charger->master,AXP81X_PDBC,0x40);
	else
		axp_set_bits(charger->master,AXP81X_PDBC,0x40);
	/* RDC initial */
	axp_read(charger->master, AXP81X_RDC0,&val);
	if((axp81x_config.pmu_battery_rdc) && (!(val & 0x40))){
		rdc = (axp81x_config.pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_write(charger->master, AXP81X_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp_write(charger->master,AXP81X_RDC1,rdc & 0x00FF);
	}

	axp_read(charger->master,AXP81X_BATCAP0,&val);
	if((axp81x_config.pmu_battery_cap) && (!(val & 0x80))){
		Cur_CoulombCounter = axp81x_config.pmu_battery_cap * 1000 / 1456;
		axp_write(charger->master, AXP81X_BATCAP0, ((Cur_CoulombCounter >> 8) | 0x80));
		axp_write(charger->master,AXP81X_BATCAP1,Cur_CoulombCounter & 0x00FF);
	}else if(!axp81x_config.pmu_battery_cap){
		axp_write(charger->master, AXP81X_BATCAP0, 0x00);
		axp_write(charger->master,AXP81X_BATCAP1,0x00);
	}

	if (0 != axp81x_config.pmu_temp_enable) {
		axp_write(charger->master,0x38,axp81x_config.pmu_charge_ltf*10/128);
		axp_write(charger->master,0x39,axp81x_config.pmu_charge_htf*10/128);
		axp_write(charger->master,0x3C,axp81x_config.pmu_discharge_ltf*10/128);
		axp_write(charger->master,0x3D,axp81x_config.pmu_discharge_htf*10/128);
	}

	return ret;
err_charger_init:
	return ret;
}

void axp81x_exit(struct axp_charger *charger)
{
	return;
}

