/*
 * Driver for the generic i2s codec
 *
 * Author:	Nat Jeffries <njeff@google.com>
 *		Copyright 2017
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>

static struct snd_soc_dai_driver pcm_generic_dai = {
	.name = "pcm-generic-hifi",
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			   SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_S32_LE
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			   SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_S32_LE
	},
};

static struct snd_soc_codec_driver soc_codec_dev_pcm_generic;

static int pcm_generic_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_pcm_generic,
			&pcm_generic_dai, 1);
}

static int pcm_generic_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static const struct of_device_id pcm_generic_of_match[] = {
	{ .compatible = "rpi,pcm-generic", },
	{ }
};
MODULE_DEVICE_TABLE(of, pcm_generic_of_match);

static struct platform_driver pcm_generic_codec_driver = {
	.probe 		= pcm_generic_probe,
	.remove 	= pcm_generic_remove,
	.driver		= {
		.name	= "pcm-generic-codec",
		.owner	= THIS_MODULE,
		.of_match_table = pcm_generic_of_match,
	},
};

module_platform_driver(pcm_generic_codec_driver);

MODULE_DESCRIPTION("ASoC generic pcm codec driver");
MODULE_AUTHOR("Nat Jeffries <njeff@google.com>");
MODULE_LICENSE("GPL v2");
