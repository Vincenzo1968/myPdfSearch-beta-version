/*
   Copyright (C) 2019 Vincenzo Lo Cicero

   Author: Vincenzo Lo Cicero.
   e-mail: vincenzolocicero68@gmail.com

   This file is part of mypdfsearch.

   mypdfsearch is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mypdfsearch is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <wchar.h>

#include "myoctal.h"

int GetOctalChar(unsigned char c1, unsigned char c2, unsigned char c3, unsigned char *pChar, int *bOctalOverflow)
{
	wchar_t wc;
	int num;
	*pChar = '\0';
	*bOctalOverflow = 0;

	switch ( c1 )
	{
		case '\0':
			return 0;
			break;
		case '0':
			switch ( c2 )
			{
			case '\0':
				wc = L'\0';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\00';
						break;
					case '0':
						wc = L'\000';
						break;
					case '1':
						wc = L'\001';
						break;
					case '2':
						wc = L'\002';
						break;
					case '3':
						wc = L'\003';
						break;
					case '4':
						wc = L'\004';
						break;
					case '5':
						wc = L'\005';
						break;
					case '6':
						wc = L'\006';
						break;
					case '7':
						wc = L'\007';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\01';
						break;
					case '0':
						wc = L'\010';
						break;
					case '1':
						wc = L'\011';
						break;
					case '2':
						wc = L'\012';
						break;
					case '3':
						wc = L'\013';
						break;
					case '4':
						wc = L'\014';
						break;
					case '5':
						wc = L'\015';
						break;
					case '6':
						wc = L'\016';
						break;
					case '7':
						wc = L'\017';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\02';
						break;
					case '0':
						wc = L'\020';
						break;
					case '1':
						wc = L'\021';
						break;
					case '2':
						wc = L'\022';
						break;
					case '3':
						wc = L'\023';
						break;
					case '4':
						wc = L'\024';
						break;
					case '5':
						wc = L'\025';
						break;
					case '6':
						wc = L'\026';
						break;
					case '7':
						wc = L'\027';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\03';
						break;
					case '0':
						wc = L'\030';
						break;
					case '1':
						wc = L'\031';
						break;
					case '2':
						wc = L'\032';
						break;
					case '3':
						wc = L'\033';
						break;
					case '4':
						wc = L'\034';
						break;
					case '5':
						wc = L'\035';
						break;
					case '6':
						wc = L'\036';
						break;
					case '7':
						wc = L'\037';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\04';
						break;
					case '0':
						wc = L'\040';
						break;
					case '1':
						wc = L'\041';
						break;
					case '2':
						wc = L'\042';
						break;
					case '3':
						wc = L'\043';
						break;
					case '4':
						wc = L'\044';
						break;
					case '5':
						wc = L'\045';
						break;
					case '6':
						wc = L'\046';
						break;
					case '7':
						wc = L'\047';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\05';
						break;
					case '0':
						wc = L'\050';
						break;
					case '1':
						wc = L'\051';
						break;
					case '2':
						wc = L'\052';
						break;
					case '3':
						wc = L'\053';
						break;
					case '4':
						wc = L'\054';
						break;
					case '5':
						wc = L'\055';
						break;
					case '6':
						wc = L'\056';
						break;
					case '7':
						wc = L'\057';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\06';
						break;
					case '0':
						wc = L'\060';
						break;
					case '1':
						wc = L'\061';
						break;
					case '2':
						wc = L'\062';
						break;
					case '3':
						wc = L'\063';
						break;
					case '4':
						wc = L'\064';
						break;
					case '5':
						wc = L'\065';
						break;
					case '6':
						wc = L'\066';
						break;
					case '7':
						wc = L'\067';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\07';
						break;
					case '0':
						wc = L'\070';
						break;
					case '1':
						wc = L'\071';
						break;
					case '2':
						wc = L'\072';
						break;
					case '3':
						wc = L'\073';
						break;
					case '4':
						wc = L'\074';
						break;
					case '5':
						wc = L'\075';
						break;
					case '6':
						wc = L'\076';
						break;
					case '7':
						wc = L'\077';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '1':
			switch ( c2 )
			{
			case '\0':
				wc = L'\1';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\10';
						break;
					case '0':
						wc = L'\100';
						break;
					case '1':
						wc = L'\101';
						break;
					case '2':
						wc = L'\102';
						break;
					case '3':
						wc = L'\103';
						break;
					case '4':
						wc = L'\104';
						break;
					case '5':
						wc = L'\105';
						break;
					case '6':
						wc = L'\106';
						break;
					case '7':
						wc = L'\107';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\11';
						break;
					case '0':
						wc = L'\110';
						break;
					case '1':
						wc = L'\111';
						break;
					case '2':
						wc = L'\112';
						break;
					case '3':
						wc = L'\113';
						break;
					case '4':
						wc = L'\114';
						break;
					case '5':
						wc = L'\115';
						break;
					case '6':
						wc = L'\116';
						break;
					case '7':
						wc = L'\117';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\12';
						break;
					case '0':
						wc = L'\120';
						break;
					case '1':
						wc = L'\121';
						break;
					case '2':
						wc = L'\122';
						break;
					case '3':
						wc = L'\123';
						break;
					case '4':
						wc = L'\124';
						break;
					case '5':
						wc = L'\125';
						break;
					case '6':
						wc = L'\126';
						break;
					case '7':
						wc = L'\127';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\13';
						break;
					case '0':
						wc = L'\130';
						break;
					case '1':
						wc = L'\131';
						break;
					case '2':
						wc = L'\132';
						break;
					case '3':
						wc = L'\133';
						break;
					case '4':
						wc = L'\134';
						break;
					case '5':
						wc = L'\135';
						break;
					case '6':
						wc = L'\136';
						break;
					case '7':
						wc = L'\137';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\14';
						break;
					case '0':
						wc = L'\140';
						break;
					case '1':
						wc = L'\141';
						break;
					case '2':
						wc = L'\142';
						break;
					case '3':
						wc = L'\143';
						break;
					case '4':
						wc = L'\144';
						break;
					case '5':
						wc = L'\145';
						break;
					case '6':
						wc = L'\146';
						break;
					case '7':
						wc = L'\147';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\15';
						break;
					case '0':
						wc = L'\150';
						break;
					case '1':
						wc = L'\151';
						break;
					case '2':
						wc = L'\152';
						break;
					case '3':
						wc = L'\153';
						break;
					case '4':
						wc = L'\154';
						break;
					case '5':
						wc = L'\155';
						break;
					case '6':
						wc = L'\156';
						break;
					case '7':
						wc = L'\157';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\16';
						break;
					case '0':
						wc = L'\160';
						break;
					case '1':
						wc = L'\161';
						break;
					case '2':
						wc = L'\162';
						break;
					case '3':
						wc = L'\163';
						break;
					case '4':
						wc = L'\164';
						break;
					case '5':
						wc = L'\165';
						break;
					case '6':
						wc = L'\166';
						break;
					case '7':
						wc = L'\167';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\17';
						break;
					case '0':
						wc = L'\170';
						break;
					case '1':
						wc = L'\171';
						break;
					case '2':
						wc = L'\172';
						break;
					case '3':
						wc = L'\173';
						break;
					case '4':
						wc = L'\174';
						break;
					case '5':
						wc = L'\175';
						break;
					case '6':
						wc = L'\176';
						break;
					case '7':
						wc = L'\177';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '2':
			switch ( c2 )
			{
			case '\0':
				wc = L'\2';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\20';
						break;
					case '0':
						wc = L'\200';
						break;
					case '1':
						wc = L'\201';
						break;
					case '2':
						wc = L'\202';
						break;
					case '3':
						wc = L'\203';
						break;
					case '4':
						wc = L'\204';
						break;
					case '5':
						wc = L'\205';
						break;
					case '6':
						wc = L'\206';
						break;
					case '7':
						wc = L'\207';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\21';
						break;
					case '0':
						wc = L'\210';
						break;
					case '1':
						wc = L'\211';
						break;
					case '2':
						wc = L'\212';
						break;
					case '3':
						wc = L'\213';
						break;
					case '4':
						wc = L'\214';
						break;
					case '5':
						wc = L'\215';
						break;
					case '6':
						wc = L'\216';
						break;
					case '7':
						wc = L'\217';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\22';
						break;
					case '0':
						wc = L'\220';
						break;
					case '1':
						wc = L'\221';
						break;
					case '2':
						wc = L'\222';
						break;
					case '3':
						wc = L'\223';
						break;
					case '4':
						wc = L'\224';
						break;
					case '5':
						wc = L'\225';
						break;
					case '6':
						wc = L'\226';
						break;
					case '7':
						wc = L'\227';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\23';
						break;
					case '0':
						wc = L'\230';
						break;
					case '1':
						wc = L'\231';
						break;
					case '2':
						wc = L'\232';
						break;
					case '3':
						wc = L'\233';
						break;
					case '4':
						wc = L'\234';
						break;
					case '5':
						wc = L'\235';
						break;
					case '6':
						wc = L'\236';
						break;
					case '7':
						wc = L'\237';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\24';
						break;
					case '0':
						wc = L'\240';
						break;
					case '1':
						wc = L'\241';
						break;
					case '2':
						wc = L'\242';
						break;
					case '3':
						wc = L'\243';
						break;
					case '4':
						wc = L'\244';
						break;
					case '5':
						wc = L'\245';
						break;
					case '6':
						wc = L'\246';
						break;
					case '7':
						wc = L'\247';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\25';
						break;
					case '0':
						wc = L'\250';
						break;
					case '1':
						wc = L'\251';
						break;
					case '2':
						wc = L'\252';
						break;
					case '3':
						wc = L'\253';
						break;
					case '4':
						wc = L'\254';
						break;
					case '5':
						wc = L'\255';
						break;
					case '6':
						wc = L'\256';
						break;
					case '7':
						wc = L'\257';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\26';
						break;
					case '0':
						wc = L'\260';
						break;
					case '1':
						wc = L'\261';
						break;
					case '2':
						wc = L'\262';
						break;
					case '3':
						wc = L'\263';
						break;
					case '4':
						wc = L'\264';
						break;
					case '5':
						wc = L'\265';
						break;
					case '6':
						wc = L'\266';
						break;
					case '7':
						wc = L'\267';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\27';
						break;
					case '0':
						wc = L'\270';
						break;
					case '1':
						wc = L'\271';
						break;
					case '2':
						wc = L'\272';
						break;
					case '3':
						wc = L'\273';
						break;
					case '4':
						wc = L'\274';
						break;
					case '5':
						wc = L'\275';
						break;
					case '6':
						wc = L'\276';
						break;
					case '7':
						wc = L'\277';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '3':
			switch ( c2 )
			{
			case '\0':
				wc = L'\3';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\30';
						break;
					case '0':
						wc = L'\300';
						break;
					case '1':
						wc = L'\301';
						break;
					case '2':
						wc = L'\302';
						break;
					case '3':
						wc = L'\303';
						break;
					case '4':
						wc = L'\304';
						break;
					case '5':
						wc = L'\305';
						break;
					case '6':
						wc = L'\306';
						break;
					case '7':
						wc = L'\307';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\31';
						break;
					case '0':
						wc = L'\310';
						break;
					case '1':
						wc = L'\311';
						break;
					case '2':
						wc = L'\312';
						break;
					case '3':
						wc = L'\313';
						break;
					case '4':
						wc = L'\314';
						break;
					case '5':
						wc = L'\315';
						break;
					case '6':
						wc = L'\316';
						break;
					case '7':
						wc = L'\317';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\32';
						break;
					case '0':
						wc = L'\320';
						break;
					case '1':
						wc = L'\321';
						break;
					case '2':
						wc = L'\322';
						break;
					case '3':
						wc = L'\323';
						break;
					case '4':
						wc = L'\324';
						break;
					case '5':
						wc = L'\325';
						break;
					case '6':
						wc = L'\326';
						break;
					case '7':
						wc = L'\327';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\33';
						break;
					case '0':
						wc = L'\330';
						break;
					case '1':
						wc = L'\331';
						break;
					case '2':
						wc = L'\332';
						break;
					case '3':
						wc = L'\333';
						break;
					case '4':
						wc = L'\334';
						break;
					case '5':
						wc = L'\335';
						break;
					case '6':
						wc = L'\336';
						break;
					case '7':
						wc = L'\337';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\34';
						break;
					case '0':
						wc = L'\340';
						break;
					case '1':
						wc = L'\341';
						break;
					case '2':
						wc = L'\342';
						break;
					case '3':
						wc = L'\343';
						break;
					case '4':
						wc = L'\344';
						break;
					case '5':
						wc = L'\345';
						break;
					case '6':
						wc = L'\346';
						break;
					case '7':
						wc = L'\347';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\35';
						break;
					case '0':
						wc = L'\350';
						break;
					case '1':
						wc = L'\351';
						break;
					case '2':
						wc = L'\352';
						break;
					case '3':
						wc = L'\353';
						break;
					case '4':
						wc = L'\354';
						break;
					case '5':
						wc = L'\355';
						break;
					case '6':
						wc = L'\356';
						break;
					case '7':
						wc = L'\357';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\36';
						break;
					case '0':
						wc = L'\360';
						break;
					case '1':
						wc = L'\361';
						break;
					case '2':
						wc = L'\362';
						break;
					case '3':
						wc = L'\363';
						break;
					case '4':
						wc = L'\364';
						break;
					case '5':
						wc = L'\365';
						break;
					case '6':
						wc = L'\366';
						break;
					case '7':
						wc = L'\367';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\37';
						break;
					case '0':
						wc = L'\370';
						break;
					case '1':
						wc = L'\371';
						break;
					case '2':
						wc = L'\372';
						break;
					case '3':
						wc = L'\373';
						break;
					case '4':
						wc = L'\374';
						break;
					case '5':
						wc = L'\375';
						break;
					case '6':
						wc = L'\376';
						break;
					case '7':
						wc = L'\377';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '4':
			switch ( c2 )
			{
			case '\0':
				wc = L'\4';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\40';
						break;
					case '0':
						wc = L'\400';
						break;
					case '1':
						wc = L'\401';
						break;
					case '2':
						wc = L'\402';
						break;
					case '3':
						wc = L'\403';
						break;
					case '4':
						wc = L'\404';
						break;
					case '5':
						wc = L'\405';
						break;
					case '6':
						wc = L'\406';
						break;
					case '7':
						wc = L'\407';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\41';
						break;
					case '0':
						wc = L'\410';
						break;
					case '1':
						wc = L'\411';
						break;
					case '2':
						wc = L'\412';
						break;
					case '3':
						wc = L'\413';
						break;
					case '4':
						wc = L'\414';
						break;
					case '5':
						wc = L'\415';
						break;
					case '6':
						wc = L'\416';
						break;
					case '7':
						wc = L'\417';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\42';
						break;
					case '0':
						wc = L'\420';
						break;
					case '1':
						wc = L'\421';
						break;
					case '2':
						wc = L'\422';
						break;
					case '3':
						wc = L'\423';
						break;
					case '4':
						wc = L'\424';
						break;
					case '5':
						wc = L'\425';
						break;
					case '6':
						wc = L'\426';
						break;
					case '7':
						wc = L'\427';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\43';
						break;
					case '0':
						wc = L'\430';
						break;
					case '1':
						wc = L'\431';
						break;
					case '2':
						wc = L'\432';
						break;
					case '3':
						wc = L'\433';
						break;
					case '4':
						wc = L'\434';
						break;
					case '5':
						wc = L'\435';
						break;
					case '6':
						wc = L'\436';
						break;
					case '7':
						wc = L'\437';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\44';
						break;
					case '0':
						wc = L'\440';
						break;
					case '1':
						wc = L'\441';
						break;
					case '2':
						wc = L'\442';
						break;
					case '3':
						wc = L'\443';
						break;
					case '4':
						wc = L'\444';
						break;
					case '5':
						wc = L'\445';
						break;
					case '6':
						wc = L'\446';
						break;
					case '7':
						wc = L'\447';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\45';
						break;
					case '0':
						wc = L'\450';
						break;
					case '1':
						wc = L'\451';
						break;
					case '2':
						wc = L'\452';
						break;
					case '3':
						wc = L'\453';
						break;
					case '4':
						wc = L'\454';
						break;
					case '5':
						wc = L'\455';
						break;
					case '6':
						wc = L'\456';
						break;
					case '7':
						wc = L'\457';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\46';
						break;
					case '0':
						wc = L'\460';
						break;
					case '1':
						wc = L'\461';
						break;
					case '2':
						wc = L'\462';
						break;
					case '3':
						wc = L'\463';
						break;
					case '4':
						wc = L'\464';
						break;
					case '5':
						wc = L'\465';
						break;
					case '6':
						wc = L'\466';
						break;
					case '7':
						wc = L'\467';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\47';
						break;
					case '0':
						wc = L'\470';
						break;
					case '1':
						wc = L'\471';
						break;
					case '2':
						wc = L'\472';
						break;
					case '3':
						wc = L'\473';
						break;
					case '4':
						wc = L'\474';
						break;
					case '5':
						wc = L'\475';
						break;
					case '6':
						wc = L'\476';
						break;
					case '7':
						wc = L'\477';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '5':
			switch ( c2 )
			{
			case '\0':
				wc = L'\5';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\50';
						break;
					case '0':
						wc = L'\500';
						break;
					case '1':
						wc = L'\501';
						break;
					case '2':
						wc = L'\502';
						break;
					case '3':
						wc = L'\503';
						break;
					case '4':
						wc = L'\504';
						break;
					case '5':
						wc = L'\505';
						break;
					case '6':
						wc = L'\506';
						break;
					case '7':
						wc = L'\507';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\51';
						break;
					case '0':
						wc = L'\510';
						break;
					case '1':
						wc = L'\511';
						break;
					case '2':
						wc = L'\512';
						break;
					case '3':
						wc = L'\513';
						break;
					case '4':
						wc = L'\514';
						break;
					case '5':
						wc = L'\515';
						break;
					case '6':
						wc = L'\516';
						break;
					case '7':
						wc = L'\517';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\52';
						break;
					case '0':
						wc = L'\520';
						break;
					case '1':
						wc = L'\521';
						break;
					case '2':
						wc = L'\522';
						break;
					case '3':
						wc = L'\523';
						break;
					case '4':
						wc = L'\524';
						break;
					case '5':
						wc = L'\525';
						break;
					case '6':
						wc = L'\526';
						break;
					case '7':
						wc = L'\527';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\53';
						break;
					case '0':
						wc = L'\530';
						break;
					case '1':
						wc = L'\531';
						break;
					case '2':
						wc = L'\532';
						break;
					case '3':
						wc = L'\533';
						break;
					case '4':
						wc = L'\534';
						break;
					case '5':
						wc = L'\535';
						break;
					case '6':
						wc = L'\536';
						break;
					case '7':
						wc = L'\537';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\54';
						break;
					case '0':
						wc = L'\540';
						break;
					case '1':
						wc = L'\541';
						break;
					case '2':
						wc = L'\542';
						break;
					case '3':
						wc = L'\543';
						break;
					case '4':
						wc = L'\544';
						break;
					case '5':
						wc = L'\545';
						break;
					case '6':
						wc = L'\546';
						break;
					case '7':
						wc = L'\547';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\55';
						break;
					case '0':
						wc = L'\550';
						break;
					case '1':
						wc = L'\551';
						break;
					case '2':
						wc = L'\552';
						break;
					case '3':
						wc = L'\553';
						break;
					case '4':
						wc = L'\554';
						break;
					case '5':
						wc = L'\555';
						break;
					case '6':
						wc = L'\556';
						break;
					case '7':
						wc = L'\557';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\56';
						break;
					case '0':
						wc = L'\560';
						break;
					case '1':
						wc = L'\561';
						break;
					case '2':
						wc = L'\562';
						break;
					case '3':
						wc = L'\563';
						break;
					case '4':
						wc = L'\564';
						break;
					case '5':
						wc = L'\565';
						break;
					case '6':
						wc = L'\566';
						break;
					case '7':
						wc = L'\567';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\57';
						break;
					case '0':
						wc = L'\570';
						break;
					case '1':
						wc = L'\571';
						break;
					case '2':
						wc = L'\572';
						break;
					case '3':
						wc = L'\573';
						break;
					case '4':
						wc = L'\574';
						break;
					case '5':
						wc = L'\575';
						break;
					case '6':
						wc = L'\576';
						break;
					case '7':
						wc = L'\577';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '6':
			switch ( c2 )
			{
			case '\0':
				wc = L'\6';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\60';
						break;
					case '0':
						wc = L'\600';
						break;
					case '1':
						wc = L'\601';
						break;
					case '2':
						wc = L'\602';
						break;
					case '3':
						wc = L'\603';
						break;
					case '4':
						wc = L'\604';
						break;
					case '5':
						wc = L'\605';
						break;
					case '6':
						wc = L'\606';
						break;
					case '7':
						wc = L'\607';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\61';
						break;
					case '0':
						wc = L'\610';
						break;
					case '1':
						wc = L'\611';
						break;
					case '2':
						wc = L'\612';
						break;
					case '3':
						wc = L'\613';
						break;
					case '4':
						wc = L'\614';
						break;
					case '5':
						wc = L'\615';
						break;
					case '6':
						wc = L'\616';
						break;
					case '7':
						wc = L'\617';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\62';
						break;
					case '0':
						wc = L'\620';
						break;
					case '1':
						wc = L'\621';
						break;
					case '2':
						wc = L'\622';
						break;
					case '3':
						wc = L'\623';
						break;
					case '4':
						wc = L'\624';
						break;
					case '5':
						wc = L'\625';
						break;
					case '6':
						wc = L'\626';
						break;
					case '7':
						wc = L'\627';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\63';
						break;
					case '0':
						wc = L'\630';
						break;
					case '1':
						wc = L'\631';
						break;
					case '2':
						wc = L'\632';
						break;
					case '3':
						wc = L'\633';
						break;
					case '4':
						wc = L'\634';
						break;
					case '5':
						wc = L'\635';
						break;
					case '6':
						wc = L'\636';
						break;
					case '7':
						wc = L'\637';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\64';
						break;
					case '0':
						wc = L'\640';
						break;
					case '1':
						wc = L'\641';
						break;
					case '2':
						wc = L'\642';
						break;
					case '3':
						wc = L'\643';
						break;
					case '4':
						wc = L'\644';
						break;
					case '5':
						wc = L'\645';
						break;
					case '6':
						wc = L'\646';
						break;
					case '7':
						wc = L'\647';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\65';
						break;
					case '0':
						wc = L'\650';
						break;
					case '1':
						wc = L'\651';
						break;
					case '2':
						wc = L'\652';
						break;
					case '3':
						wc = L'\653';
						break;
					case '4':
						wc = L'\654';
						break;
					case '5':
						wc = L'\655';
						break;
					case '6':
						wc = L'\656';
						break;
					case '7':
						wc = L'\657';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\66';
						break;
					case '0':
						wc = L'\660';
						break;
					case '1':
						wc = L'\661';
						break;
					case '2':
						wc = L'\662';
						break;
					case '3':
						wc = L'\663';
						break;
					case '4':
						wc = L'\664';
						break;
					case '5':
						wc = L'\665';
						break;
					case '6':
						wc = L'\666';
						break;
					case '7':
						wc = L'\667';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\67';
						break;
					case '0':
						wc = L'\670';
						break;
					case '1':
						wc = L'\671';
						break;
					case '2':
						wc = L'\672';
						break;
					case '3':
						wc = L'\673';
						break;
					case '4':
						wc = L'\674';
						break;
					case '5':
						wc = L'\675';
						break;
					case '6':
						wc = L'\676';
						break;
					case '7':
						wc = L'\677';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		case '7':
			switch ( c2 )
			{
			case '\0':
				wc = L'\7';
				break;
			case '0':
				switch ( c3 )
				{
					case '\0':
						wc = L'\70';
						break;
					case '0':
						wc = L'\700';
						break;
					case '1':
						wc = L'\701';
						break;
					case '2':
						wc = L'\702';
						break;
					case '3':
						wc = L'\703';
						break;
					case '4':
						wc = L'\704';
						break;
					case '5':
						wc = L'\705';
						break;
					case '6':
						wc = L'\706';
						break;
					case '7':
						wc = L'\707';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '1':
				switch ( c3 )
				{
					case '\0':
						wc = L'\71';
						break;
					case '0':
						wc = L'\710';
						break;
					case '1':
						wc = L'\711';
						break;
					case '2':
						wc = L'\712';
						break;
					case '3':
						wc = L'\713';
						break;
					case '4':
						wc = L'\714';
						break;
					case '5':
						wc = L'\715';
						break;
					case '6':
						wc = L'\716';
						break;
					case '7':
						wc = L'\717';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '2':
				switch ( c3 )
				{
					case '\0':
						wc = L'\72';
						break;
					case '0':
						wc = L'\720';
						break;
					case '1':
						wc = L'\721';
						break;
					case '2':
						wc = L'\722';
						break;
					case '3':
						wc = L'\723';
						break;
					case '4':
						wc = L'\724';
						break;
					case '5':
						wc = L'\725';
						break;
					case '6':
						wc = L'\726';
						break;
					case '7':
						wc = L'\727';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '3':
				switch ( c3 )
				{
					case '\0':
						wc = L'\73';
						break;
					case '0':
						wc = L'\730';
						break;
					case '1':
						wc = L'\731';
						break;
					case '2':
						wc = L'\732';
						break;
					case '3':
						wc = L'\733';
						break;
					case '4':
						wc = L'\734';
						break;
					case '5':
						wc = L'\735';
						break;
					case '6':
						wc = L'\736';
						break;
					case '7':
						wc = L'\737';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '4':
				switch ( c3 )
				{
					case '\0':
						wc = L'\74';
						break;
					case '0':
						wc = L'\740';
						break;
					case '1':
						wc = L'\741';
						break;
					case '2':
						wc = L'\742';
						break;
					case '3':
						wc = L'\743';
						break;
					case '4':
						wc = L'\744';
						break;
					case '5':
						wc = L'\745';
						break;
					case '6':
						wc = L'\746';
						break;
					case '7':
						wc = L'\747';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '5':
				switch ( c3 )
				{
					case '\0':
						wc = L'\75';
						break;
					case '0':
						wc = L'\750';
						break;
					case '1':
						wc = L'\751';
						break;
					case '2':
						wc = L'\752';
						break;
					case '3':
						wc = L'\753';
						break;
					case '4':
						wc = L'\754';
						break;
					case '5':
						wc = L'\755';
						break;
					case '6':
						wc = L'\756';
						break;
					case '7':
						wc = L'\757';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '6':
				switch ( c3 )
				{
					case '\0':
						wc = L'\76';
						break;
					case '0':
						wc = L'\760';
						break;
					case '1':
						wc = L'\761';
						break;
					case '2':
						wc = L'\762';
						break;
					case '3':
						wc = L'\763';
						break;
					case '4':
						wc = L'\764';
						break;
					case '5':
						wc = L'\765';
						break;
					case '6':
						wc = L'\766';
						break;
					case '7':
						wc = L'\767';
						break;
					default:
						return 0;
						break;
				}
				break;
			case '7':
				switch ( c3 )
				{
					case '\0':
						wc = L'\77';
						break;
					case '0':
						wc = L'\770';
						break;
					case '1':
						wc = L'\771';
						break;
					case '2':
						wc = L'\772';
						break;
					case '3':
						wc = L'\773';
						break;
					case '4':
						wc = L'\774';
						break;
					case '5':
						wc = L'\775';
						break;
					case '6':
						wc = L'\776';
						break;
					case '7':
						wc = L'\777';
						break;
					default:
						return 0;
						break;
				}
				break;
			default:
				return 0;
				break;
			}
			break;
		default:
			return 0;
			break;
	}

	num = (int)wc;
	if ( num > 255 )
	{
		*pChar = '\0';
		*bOctalOverflow = 1;
		return 0;
	}

	*pChar = (char)wc;

	return 1;
}
