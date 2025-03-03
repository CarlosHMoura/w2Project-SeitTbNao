#include "cServer.h"
#include "Basedef.h"
#include "GetFunc.h"
#include "SendFunc.h"
#include "BufferReaderWriter.h"

void SendSignal(INT32 toClientId, INT32 clientId, INT16 packetId)
{
	if (toClientId <= 0 || toClientId >= MAX_PLAYER)
		return;

	PacketHeader header;
	memset(&header, 0, sizeof header);

	header.PacketId = packetId;
	header.ClientId = clientId;
	header.Size = sizeof header;

	pUser[toClientId].AddMessage((BYTE*)&header, sizeof header);
}

void SendSignalParm(INT32 toClientId, INT32 clientId, INT16 packetId, INT32 value)
{
	if (toClientId <= 0 || toClientId >= MAX_PLAYER)
		return;

	pMsgSignal packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = packetId;
	packet.Header.ClientId = clientId;
	packet.Header.Size = sizeof packet;

	packet.Value = value;

	pUser[toClientId].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendSignalParm2(INT32 toClientId, INT32 clientId, INT16 packetId, INT32 value, INT32 value2)
{
	if (toClientId <= 0 || toClientId >= MAX_PLAYER)
		return;

	pMsgSignal2 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = packetId;
	packet.Header.ClientId = clientId;
	packet.Header.Size = sizeof packet;

	packet.Value = value;
	packet.Value2 = value2;

	pUser[toClientId].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendSignalParm3(INT32 toClientId, INT32 clientId, INT16 packetId, INT32 value, INT32 value2, INT32 value3)
{
	if (toClientId <= 0 || toClientId >= MAX_PLAYER)
		return;

	pMsgSignal3 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = packetId;
	packet.Header.ClientId = clientId;
	packet.Header.Size = sizeof packet;

	packet.Value = value;
	packet.Value2 = value2;
	packet.Value3 = value3;

	pUser[toClientId].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendKingdomBattleInfo(int clientId, int kingdom, bool status)
{
	_MSG_REALBATTLE packet{};
	packet.Header.PacketId = RealBattlePacket;
	packet.Header.ClientId = clientId;

	packet.Kingdom = kingdom;
	packet.Status = status;

	if (clientId <= 0 || clientId >= MAX_PLAYER)
	{
		for (auto& user : pUser)
		{
			if (user.Status == USER_PLAY)
				user.AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof _MSG_REALBATTLE);
		}

		return;
	}

	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof _MSG_REALBATTLE);
}

void SendGridMob(int Index)
{
	CMob *mob = (CMob*)&pMob[Index];

	int VisX = VIEWGRIDX, VisY = VIEWGRIDY,
		minPosX = (mob->Target.X - HALFGRIDX),
		minPosY = (mob->Target.Y - HALFGRIDY);

	if ((minPosX + VisX) >= 4096)
		VisX = (VisX - (VisX + minPosX - 4096));

	if ((minPosY + VisY) >= 4096)
		VisY = (VisY - (VisY + minPosY - 4096));

	if (minPosX < 0)
	{
		minPosX = 0;
		VisX = (VisX + minPosX);
	}

	if (minPosY < 0)
	{
		minPosY = 0;
		VisY = (VisY + minPosY);
	}

	int maxPosX = (minPosX + VisX),
		maxPosY = (minPosY + VisY);

	for (int nY = minPosY; nY < maxPosY; nY++)
	{
		for (int nX = minPosX; nX < maxPosX; nX++)
		{
			short mobID = g_pMobGrid[nY][nX];
			short initID = g_pItemGrid[nY][nX];

			if (mobID != 0 && Index != mobID)
			{
				if (mobID < MAX_PLAYER)
					SendCreateMob(mobID, Index);

				if (Index < MAX_PLAYER)
					SendCreateMob(Index, mobID);
			}

			if (initID > 0 && mobID < MAX_PLAYER)
				SendCreateItem(mobID, initID, 0);

			if (initID > 0 && Index < MAX_PLAYER)
				SendCreateItem(Index, initID, 0);
		}
	}
}

void SendCreateMob(int sendClientID, int createClientID, INT32 send)
{
	if (sendClientID >= MAX_PLAYER || sendClientID <= 0)
		return;

	if (createClientID < MAX_PLAYER && pUser[createClientID].IsAutoTrading)
	{ // Envia a venda do player
		p363 pak;
		GetCreateMobTrade(createClientID, (BYTE*)&pak);

		pUser[sendClientID].AddMessage((BYTE*)&pak, sizeof pak);

		if (send != 0)
			pUser[sendClientID].SendMessageA();
	}
	else
	{ // Envia o spawn normal
		if (createClientID > 0 && createClientID < MAX_PLAYER && pUser[createClientID].IsBanned == 1 && !pUser[sendClientID].IsAdmin)
			return;

		p364 packet{};
		GetCreateMob(createClientID, (BYTE*)&packet);

		pUser[sendClientID].AddMessage((BYTE*)&packet, sizeof packet);

		if (send != 0)
			pUser[sendClientID].SendMessageA();
	}
}

void SendCounterMob(int clientId, short value, short total)
{
	p3BB packet;
	memset(&packet, 0, sizeof p3BB);

	packet.Header.Size = sizeof p3BB;
	packet.Header.PacketId = 0x3BB;
	packet.Header.ClientId = clientId;

	packet.Value = value;
	packet.Total = total;

	pUser[clientId].AddMessage((BYTE*)&packet, sizeof p3BB);
}

void SendCounterMobArea(int value1, int value2, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	for (int i = 0; i < MAX_PLAYER; i++)
	{
		if (pUser[i].Status != USER_PLAY)
			continue;

		if (pMob[i].Target.X >= x1 && pMob[i].Target.Y >= y1 &&
			pMob[i].Target.X <= x2 && pMob[i].Target.Y <= y2)
		{
			SendCounterMob(i, value1, value2);
		}
	}
}

void SendAffect(int clientId)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER)
		return;

	p3B9 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x3B9;
	packet.Header.ClientId = clientId;
	packet.Header.Size = sizeof p3B9;

	STRUCT_AFFECT *affect = pMob[clientId].Mobs.Affects;
	// Passa a divina pra frente
	for (int i = 1; i < 32; i++)
	{
		if (affect[i].Index == 34) // DIVINA
		{
			// Buffer temporario para salvar o buff atual
			STRUCT_AFFECT tmpAffect;

			memcpy(&tmpAffect, &affect[0], sizeof STRUCT_AFFECT);
			memcpy(&affect[0], &affect[i], sizeof STRUCT_AFFECT);
			memcpy(&affect[i], &tmpAffect, sizeof STRUCT_AFFECT);
			break;
		}
	}

	// Caso tenha ainda divina restante, atribui a estrutura
	float timeDiv = TimeRemaining(pUser[clientId].User.Divina);
	if (timeDiv > 0.0f)
	{
		// Checa se ja ha divina na estrutura
		int i;
		for (i = 0; i < 32; i++)
		{
			if (affect[i].Index == 34)
				break;
		}

		// Caso retorne 32, quer dizer que ele possui divina ativada por�m n�o esta 
		// buffado, ent�o buffara sozinho
		if (i == 32)
		{
			for (i = 0; i < 32; i++)
			{
				if (affect[i].Index == 0)
				{
					affect[i].Index = 34;
					affect[i].Master = 1;
					affect[i].Time = 999;

					break;
				}
			}
		}
	}

	float timeSeph = TimeRemaining(pUser[clientId].User.Sephira);
	if (timeSeph > 0.0f)
	{
		// Checa se ja ha sephira na estrutura
		int i;
		for (i = 0; i < 32; i++)
		{
			if (affect[i].Index == 4)
				break;
		}

		// Caso retorne 32, quer dizer que ele possui sephira ativada por�m n�o esta 
		// buffado, ent�o buffara sozinho
		if (i == 32)
		{
			for (i = 0; i < 32; i++)
			{
				if (affect[i].Index == 0)
				{
					affect[i].Index = 4;
					affect[i].Master = 1;
					affect[i].Value = 4;
					affect[i].Time = 999;
					break;
				}
			}
		}
	}

	float timeSaude = TimeRemaining(pMob[clientId].Mobs.Saude);
	if (timeSaude > 0.0f)
	{
		// Checa se ja ha sa�de na estrutura
		int i;
		for (i = 0; i < 32; i++)
		{
			if (affect[i].Index == 35)
				break;
		}

		// Caso retorne 32, quer dizer que ele possui sa�de ativada por�m n�o esta 
		// buffado, ent�o buffara sozinho
		if (i == 32)
		{
			for (i = 0; i < 32; i++)
			{
				if (affect[i].Index == 0)
				{
					affect[i].Index = 35;
					affect[i].Master = 1;
					affect[i].Value = 1;
					affect[i].Time = 999;
					break;
				}
			}
		}
	}

	float timeRevi = TimeRemaining(pMob[clientId].Mobs.Revigorante);
	if (timeRevi > 0.0f)
	{
		// Checa se ja ha sa�de na estrutura
		int i;
		for (i = 0; i < 32; i++)
		{
			if (affect[i].Index == 51)
				break;
		}

		// Caso retorne 32, quer dizer que ele possui sa�de ativada por�m n�o esta 
		// buffado, ent�o buffara sozinho
		if (i == 32)
		{
			for (i = 0; i < 32; i++)
			{
				if (affect[i].Index == 0)
				{
					affect[i].Index = 51;
					affect[i].Time = 999;

					break;
				}
			}
		}
	}

	for (INT32 i = 0; i < 32; i++)
	{
		if (affect[i].Index == 0 || affect[i].Time <= 0)
			continue;

		packet.Affect[i].Index = affect[i].Index;
		packet.Affect[i].Value = affect[i].Value;
		packet.Affect[i].Master = affect[i].Master;
		packet.Affect[i].Time = affect[i].Time;

		// Divina
		if (affect[i].Index == 34)
		{
			if (timeDiv <= 0)
			{
				memset(&affect[i], 0, sizeof STRUCT_AFFECT);

				continue;
			}

			affect[i].Time = 999;

			packet.Affect[i].Time = static_cast<int>(timeDiv);
		}
		else if (affect[i].Index == 4 && (affect[i].Value != 6 && affect[i].Value != 7 && affect[i].Value != 8))
		{
			if (timeSeph <= 0)
			{
				memset(&affect[i], 0, sizeof STRUCT_AFFECT);

				continue;
			}

			affect[i].Time = 999;

			packet.Affect[i].Time = static_cast<int>(timeSeph);
		}
		else if (affect[i].Index == 35 && affect[i].Value == 1)
		{
			if (timeSaude <= 0)
			{
				memset(&affect[i], 0, sizeof STRUCT_AFFECT);

				continue;
			}

			affect[i].Time = 999;

			packet.Affect[i].Time = static_cast<int>(timeSaude);
		}
		else if (affect[i].Index == 51)
		{
			if (timeRevi <= 0)
			{
				memset(&affect[i], 0, sizeof STRUCT_AFFECT);

				continue;
			}

			affect[i].Time = 999;

			packet.Affect[i].Time = static_cast<int>(timeRevi);
		}
	}

	pUser[clientId].AddMessage((BYTE*)&packet, sizeof p3B9);
}

void SendItem(int clientId, SlotType invType, int slotId, STRUCT_ITEM *item)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER)
		return;

	p182 p;

	p.Header.ClientId = clientId;
	p.Header.PacketId = 0x182;
	p.Header.Size = sizeof p182;

	p.invType = (short)invType;
	p.invSlot = slotId;

	memcpy(&p.itemData, item, sizeof STRUCT_ITEM);

	pUser[clientId].AddMessage((BYTE*)&p, sizeof p182);

	//	SendEquip(clientId);
}

void SendEquip(int clientId)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	p36B p;
	memset(&p, 0, sizeof p36B);
	p.Header.ClientId = clientId;
	p.Header.PacketId = 0x36B;
	p.Header.Size = sizeof p36B;

	STRUCT_MOB *mob = (STRUCT_MOB*)(&pMob[clientId].Mobs.Player);
	bool isUsingCostume = mob->Equip[12].Index != 0;
	for (int i = 0; i < 16; i++)
	{
		short effValue = 0;
		STRUCT_ITEM item = mob->Equip[i];

		if (i == 14)
		{
			if (item.Index >= 2360 && item.Index <= 2389)
			{
				if (*(short*)&item.EF1 <= 0)
				{
					p.ItemEff[i] = 0;
					p.pAnctCode[i] = 0;

					continue;
				}
			}
		}

		if (i == 0)
		{
			// se tiver um traje equipado
			if (mob->Equip[12].Index != 0 && mob->ClassInfo == 2)
				item.Index = item.EF2;
		}

		p.ItemEff[i] = GetItemIDAndEffect(&item, i, isUsingCostume);
		p.pAnctCode[i] = GetAnctCode(&item, isUsingCostume);
	}

	GridMulticast_2(pMob[clientId].Target.X, pMob[clientId].Target.Y, (BYTE*)&p, 0);

}

#pragma pack(push, 1)
struct p3366
{
	PacketHeader Header; // 0 - 11
	STRUCT_STATUS   Status;

	unsigned char  Critical;
	unsigned char  SaveMana;

	struct
	{
		BYTE Time;
		BYTE Index;
	} Affect[32]; //62 - 125

	unsigned short GuildIndex;
	unsigned short GuildLevel;

	BYTE Resist1; // 130 
	BYTE Resist2; // 131
	BYTE Resist3; // 132
	BYTE Resist4; // 133

	unsigned char RegenHP;
	unsigned char RegenMP;

	int CurrHp;
	int CurrMp;

	int OldMagic;
	unsigned char Special[4];

	int Life;
	int Magic;
};
#pragma pack(pop)

void SendScore(int clientIndex)
{
	STRUCT_MOB *mob = &pMob[clientIndex].Mobs.Player;

	p3366 p;
	memset(&p, 0, sizeof p);
	p.Header.ClientId = clientIndex;
	p.Header.PacketId = 0x336;
	p.Header.Size = sizeof p3366;

	if (clientIndex < MAX_PLAYER)
	{
		p.Life = pMob[clientIndex].Lifes;

		p.CurrHp = mob->Status.curHP;
		p.CurrMp = mob->Status.curMP;
	}

	p.GuildIndex = (mob->GuildIndex);
	p.GuildLevel = mob->GuildMemberType;

	if (pMob[clientIndex].GuildDisable != 0)
		p.GuildIndex = 0;

	p.Magic = pMob[clientIndex].MagicIncrement;

	p.RegenHP = mob->RegenHP;
	p.RegenHP = mob->RegenMP;

	p.Critical = mob->Critical;
	p.Resist1 = mob->Resist.Fogo;
	p.Resist2 = mob->Resist.Gelo;
	p.Resist3 = mob->Resist.Sagrado;
	p.Resist4 = mob->Resist.Trovao;

	p.SaveMana = mob->SaveMana;

	memcpy(&p.Status, &mob->Status, sizeof STRUCT_STATUS);

	STRUCT_AFFECT *affect = pMob[clientIndex].Mobs.Affects;
	for (int i = 0; i < 32; i++)
	{
		p.Affect[i].Index = affect[i].Index & 0xFF;
		p.Affect[i].Time = affect[i].Time & 0xFF;
	}

	if (pMob[clientIndex].Mobs.Player.Info.Merchant & 1)
		p.Status.Merchant.Merchant = 1;

	if (clientIndex < MAX_PLAYER && pUser[clientIndex].Arena.GroupIndex != -1 && pMob[clientIndex].Target.X >= 143 && pMob[clientIndex].Target.Y >= 546 && pMob[clientIndex].Target.X <= 195 && pMob[clientIndex].Target.Y <= 625)
	{
		p.GuildIndex = 0;
		p.GuildLevel = 0;
	}

	GridMulticast_2(pMob[clientIndex].Target.X, pMob[clientIndex].Target.Y, (BYTE*)&p, 0);
	SendAffect(clientIndex);
}

void SendAutoTrade(int sendClientId, int tradeClientId)
{
	if (!pUser[tradeClientId].IsAutoTrading)
		return;

	p397 pTrade;
	memset(&pTrade, 0, sizeof pTrade);

	pTrade.Header.Size = sizeof(p397);
	pTrade.Header.PacketId = 0x397;
	pTrade.Header.ClientId = tradeClientId;

	pTrade.Index = tradeClientId;

	memcpy(pTrade.Gold, pUser[tradeClientId].AutoTrade.Price, sizeof(int) * 12);
	memcpy(pTrade.Item, pUser[tradeClientId].AutoTrade.Item, 12 * sizeof(STRUCT_ITEM));
	memcpy(pTrade.Slot, pUser[tradeClientId].AutoTrade.Slots, 8);
	strncpy_s(pTrade.Name, pUser[tradeClientId].AutoTrade.Name, 24);
	pTrade.Unknown = pUser[tradeClientId].AutoTrade.Unknown_1784;

	pUser[sendClientId].AddMessage((BYTE*)&pTrade, sizeof p397);
}

void SendCargoCoin(int clientId)
{
	if (clientId <= MOB_EMPTY || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	p339 p;
	memset(&p, 0, sizeof p);

	p.Header.ClientId = clientId;
	p.Header.PacketId = 0x339;

	p.Gold = pUser[clientId].User.Storage.Coin;

	pUser[clientId].AddMessage((BYTE*)&p, sizeof p);
}

void SendEtc(int clientId)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	STRUCT_MOB *mob = &pMob[clientId].Mobs.Player;
	p337 p;
	memset(&p, 0, sizeof p);

	p.Header.ClientId = clientId;
	p.Header.PacketId = 0x337;

	p.Gold = mob->Gold;
	p.Exp = mob->Exp;

	p.Hold = static_cast<unsigned int>(pMob[clientId].Mobs.Hold);

	p.Learn = mob->Learn[0];
	p.SecLearn = mob->Learn[1];
	p.Magic = mob->MagicIncrement;

	p.pMaster = mob->MasterPoint;
	p.pSkills = mob->SkillPoint;
	p.pStatus = mob->StatusPoint;

	pUser[clientId].AddMessage((BYTE*)&p, sizeof p337);
}

void DeleteMob(int clientId, int reason)
{
	pMsgSignal packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x165;
	packet.Header.ClientId = clientId;
	packet.Header.Size = 16;

	packet.Value = reason;

	GridMulticast_2(pMob[clientId].Target.X, pMob[clientId].Target.Y, (BYTE*)&packet, 0);

	if (reason != 0)
	{
		if (clientId >= MAX_PLAYER)
		{
			INT32 LOCAL_5 = pMob[clientId].GenerateID;

			if (LOCAL_5 >= 0 && LOCAL_5 < MAX_NPCGENERATOR)
			{
				mGener.pList[LOCAL_5].MobCount -= 1;

				if (mGener.pList[LOCAL_5].MobCount < 0)
					mGener.pList[LOCAL_5].MobCount = 0;
			}
		}

		pMob[clientId].Mode = 0;

		if (pMob[clientId].Target.Y >= 0 && pMob[clientId].Target.Y < 4096 && pMob[clientId].Target.X >= 0 && pMob[clientId].Target.X < 4096)
			g_pMobGrid[pMob[clientId].Target.Y][pMob[clientId].Target.X] = 0;

		RemoveParty(clientId);
	}
}

void SendSay(int clientId, const char *msg, ...)
{
	/* Arglist */
	char buffer[108];
	va_list arglist;
	va_start(arglist, msg);
	vsprintf_s(buffer, msg, arglist);
	va_end(arglist);
	/* Fim arlist */

	p333 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x333;
	packet.Header.Size = sizeof p333;
	packet.Header.ClientId = clientId;

	strncpy_s(packet.eChat, buffer, 96);

	GridMulticast_2(pMob[clientId].Target.X, pMob[clientId].Target.Y, (BYTE*)&packet, 0);
}

void SendRemoveMob(UINT16 killerId, UINT16 killedId, UINT32 mType, INT32 send)
{
	pMsgSignal p;
	memset(&p, 0, sizeof pMsgSignal);

	p.Header.ClientId = killedId;
	p.Header.PacketId = 0x165;
	p.Header.Size = sizeof pMsgSignal;

	p.Value = mType;

	pUser[killerId].AddMessage((BYTE*)&p, sizeof p);

	if (send != 0)
		pUser[killerId].SendMessageA();
}

void SendEmotion(int clientId, int val1, int val2)
{
	p36A packet;
	memset(&packet, 0, sizeof p36A);

	packet.Header.Size = sizeof p36A;
	packet.Header.PacketId = 0x36A;
	packet.Header.ClientId = clientId;

	packet.NotUsed = 0;
	packet.Motion = val1;
	packet.Parm = val2;

	GridMulticast_2(pMob[clientId].Target.X, pMob[clientId].Target.Y, (BYTE*)(&packet), 0);
}

void SendNoticeArea(const char *Message, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{//0x00401177;
	int LOCAL_1 = 1;
	for (; LOCAL_1 < MAX_PLAYER; LOCAL_1++)
	{
		if (pUser[LOCAL_1].Status != USER_PLAY)
			continue;

		if (pMob[LOCAL_1].Target.X >= x1 && pMob[LOCAL_1].Target.X <= x2
			&& pMob[LOCAL_1].Target.Y >= y1 && pMob[LOCAL_1].Target.Y <= y2)
			SendClientMessage(LOCAL_1, Message);
	}
}

void SendSetHpMp(int clientId)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	if (pUser[clientId].Socket.Socket <= 0)
		return;

	p181 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x181;
	packet.Header.Size = sizeof packet;
	packet.Header.ClientId = clientId;

	SetReqHp(clientId);
	SetReqMp(clientId);
	packet.curHP = pMob[clientId].Mobs.Player.Status.curHP;
	packet.curMP = pMob[clientId].Mobs.Player.Status.curMP;
	packet.maxHP = pUser[clientId].Potion.CountHp;
	packet.maxMP = pUser[clientId].Potion.CountMp;

	pUser[clientId].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendCreateItem(int toClientId, int initId, int unk)
{
	if (toClientId <= 0 || toClientId >= MAX_PLAYER || pUser[toClientId].Status != USER_PLAY)
		return;

	p26E pak;
	GetCreateItem(initId, &pak);

	pUser[toClientId].AddMessage((BYTE*)&pak, sizeof p26E);
}

void SendRemoveItem(int dest, int itemid, int bSend)
{
	MSG_DecayItem sm_deci;
	memset(&sm_deci, 0, sizeof(MSG_DecayItem));

	sm_deci.Header.PacketId = 0x16F;
	sm_deci.Header.Size = sizeof(MSG_DecayItem);
	sm_deci.Header.ClientId = 30000;
	sm_deci.ItemID = 10000 + itemid;
	sm_deci.unk = 0;

	pUser[dest].AddMessage((BYTE*)&sm_deci, sizeof(MSG_DecayItem));

	if (bSend)
		pUser[dest].SendMessageA();
}

void SendAddParty(int target, int whom, int leader)
{
	if (target <= 0 || target >= MAX_PLAYER || pUser[target].Status != USER_PLAY)
		return;

	p37D packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x37D;
	packet.Header.Size = sizeof packet;
	packet.Header.ClientId = 30000;

	packet.PartyID = static_cast<unsigned short>(whom);
	packet.Level = static_cast<unsigned short>(pMob[whom].Mobs.Player.bStatus.Level);
	strcpy_s(packet.nickName, pMob[whom].Mobs.Player.Name);

	if (leader)
		packet.LiderID = 0;
	else
		packet.LiderID = 30000;

	// O HP nos pacotes v�o s�o do tamanho de 2 bytes, sendo assim, se for unsigned at� 65535 e se for signed
	// at� 32767. Na atualiza��o de vers�o do WYD para a vers�o 759+, os HPs foram alterados para 4 bytes, um
	// limite bem maior que o anterior. Neste caso, acho que os kr esqueceram de atualizar este pacote... Neste
	// caso, aqui so teremos a porcentagem na barar de grupo do personagem, ent�o faremos um calculo de quantos %
	// do HP do usuar�o esta para mostrarmos na tela (n�o mostrara o valor real)
	int hpPercent = static_cast<int>((static_cast<float>(pMob[whom].Mobs.Player.Status.curHP) / static_cast<float>(pMob[whom].Mobs.Player.Status.maxHP) * 100.0f));

	packet.maxHP = 100;
	packet.curHP = hpPercent;

	packet.ID = 52428;

	pUser[target].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendRemoveParty(int target, int whom)
{
	if (target <= 0 || target >= MAX_PLAYER || pUser[target].Status != USER_PLAY || pUser[target].Socket.Socket <= 0)
		return;

	p37E packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x37E;
	packet.Header.Size = sizeof packet;
	packet.Header.ClientId = 30000;

	packet.mobId = whom;

	pUser[target].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendHpMode(int clientId)
{ // 004428E0
	if (clientId <= 0 || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	p292 p;
	p.Header.PacketId = 0x292;
	p.Header.Size = 16;
	p.Header.ClientId = clientId;

	p.CurHP = pMob[clientId].Mobs.Player.Status.curHP;
	p.Status = pUser[clientId].Status;

	pUser[clientId].AddMessage((BYTE*)&p, sizeof p292);
}

void SendCarry(int clientId)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER || pUser[clientId].Status != USER_PLAY)
		return;

	p185 packet;
	packet.Header.ClientId = clientId;
	packet.Header.PacketId = 0x185;
	packet.Header.Size = sizeof p185;

	memcpy(packet.Item, pMob[clientId].Mobs.Player.Inventory, sizeof STRUCT_ITEM * 64);

	packet.Gold = pMob[clientId].Mobs.Player.Gold;
	pUser[clientId].AddMessage((BYTE*)&packet, sizeof packet);
}

void SendDamage(unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y)
{//04012A8
	for (INT32 LOCAL_1 = 1; LOCAL_1 < MAX_PLAYER; LOCAL_1++)
	{
		if (pUser[LOCAL_1].Status != USER_PLAY)
			continue;

		if (pMob[LOCAL_1].Target.X >= min_x && pMob[LOCAL_1].Target.X <= max_x && pMob[LOCAL_1].Target.Y >= min_y &&
			pMob[LOCAL_1].Target.Y <= max_y)
		{
			float LOCAL_2 = 0.25;
			INT32 LOCAL_3 = 2000;
			INT32 LOCAL_4 = pMob[LOCAL_1].Mobs.Player.Status.curHP;
			INT32 LOCAL_5 = LOCAL_4 - LOCAL_3;
			LOCAL_4 = LOCAL_4 * LOCAL_2;

			if (LOCAL_4 < LOCAL_5)
				LOCAL_4 = LOCAL_5;

			if (LOCAL_4 < 1)
				LOCAL_4 = 1;

			INT32 LOCAL_6 = LOCAL_4 - pMob[LOCAL_1].Mobs.Player.Status.curHP;

			pMob[LOCAL_1].Mobs.Player.Status.curHP = LOCAL_4;
			pUser[LOCAL_1].Potion.CountHp += LOCAL_6;

			SetReqHp(LOCAL_1);
			SetReqMp(LOCAL_1);

			p18A packet{};
			packet.Header.PacketId = 0x18A;
			packet.Header.Size = sizeof p18A;
			packet.Header.ClientId = LOCAL_1;

			packet.CurHP = LOCAL_4;
			packet.Incress = LOCAL_6;

			INT32 LOCAL_11 = pMob[LOCAL_1].Target.X;
			INT32 LOCAL_12 = pMob[LOCAL_1].Target.Y;

			GridMulticast_2(LOCAL_11, LOCAL_12, (BYTE*)&packet, 0);
		}
	}
}

// EBP + 8, EBP + 0C, EBP + 10, EBP + 14, EBP + 18, EBP + 1C
void SendEnvEffect(int min_x, int min_y, int max_x, int max_y, int type1, int type2)
{//04012A8
	p3A2 packet;

	packet.Header.PacketId = 0x3A2;
	packet.Header.Size = sizeof p3A2;
	packet.Header.ClientId = 0x7530;

	packet.MinX = min_x;
	packet.MaxX = max_x;
	packet.MinY = min_y;
	packet.MaxY = max_y;

	packet.Type1 = type1;
	packet.Type2 = type2;

	GridMulticast_2(min_x + ((max_x - min_x) >> 1), min_y + ((max_y - min_y) >> 1), (BYTE*)&packet, 0);
}

void SendNotice(const char* msg, ...)
{
	/* Arglist */
	char buffer[256];
	va_list arglist;
	va_start(arglist, msg);
	vsprintf_s(buffer, msg, arglist);
	va_end(arglist);
	/* Fim arlist */

	buffer[107] = '\0';

	p101 packet;
	memset(&packet, 0, sizeof p101);

	packet.Header.PacketId = 0x101;
	packet.Header.ClientId = 0;
	packet.Header.Size = sizeof p101;

	strncpy_s(packet.Msg, buffer, 128);

	bool needLog = buffer[0] != '.';
	for (INT32 i = 1; i < MAX_PLAYER; i++)
	{
		if (pUser[i].Status != USER_PLAY)
			continue;

		pUser[i].AddMessage((BYTE*)&packet, sizeof p101);

		if(needLog)
			Log(i, LOG_INGAME, "[Mensagem do servidor]: %s", buffer);
	}
}

void SendGuildNotice(INT32 guildId, const char *msg, ...)
{
	/* Arglist */
	char buffer[108];
	va_list arglist;
	va_start(arglist, msg);
	vsprintf_s(buffer, msg, arglist);
	va_end(arglist);
	/* Fim arlist */

	p101 packet;
	memset(&packet, 0, sizeof p101);

	packet.Header.PacketId = 0x101;
	packet.Header.ClientId = 0;
	packet.Header.Size = sizeof p101;

	strncpy_s(packet.Msg, buffer, 96);

	for (INT32 i = 1; i < MAX_PLAYER; i++)
	{
		if (pUser[i].Status != USER_PLAY)
			continue;

		if (pMob[i].Mobs.Player.GuildIndex != guildId)
			continue;

		pUser[i].AddMessage((BYTE*)&packet, sizeof p101);
	}
}

// Notifica a DBSRV para enviar a mensagem para todos os canais
void SendServerNotice(const char *msg, ...)
{
	/* Arglist */
	char buffer[108];
	va_list arglist;
	va_start(arglist, msg);
	vsprintf_s(buffer, msg, arglist);
	va_end(arglist);

	_MSG_SEND_SERVER_NOTICE p;
	memset(&p, 0, sizeof _MSG_SEND_SERVER_NOTICE);

	p.Header.ClientId = 0;
	p.Header.PacketId = MSG_SEND_SERVER_NOTICE;
	p.Header.Size = sizeof _MSG_SEND_SERVER_NOTICE;

	strncpy_s(p.Notice, buffer, 96);

	AddMessageDB((BYTE*)&p, sizeof _MSG_SEND_SERVER_NOTICE);
}

void SendWarInfo(INT32 clientId, INT32 capeWin)
{
	INT32 LOCAL_1 = 0;
	INT32 LOCAL_2 = MAX_GUILD;
	INT32 LOCAL_3 = pMob[clientId].Mobs.Player.GuildIndex;

	if (LOCAL_3 <= 0 || LOCAL_3 >= LOCAL_2)
		LOCAL_3 = 0;

	INT32 LOCAL_4 = g_pGuildWar[LOCAL_3];
	if (LOCAL_4 <= 0 || LOCAL_4 >= LOCAL_2)
		LOCAL_4 = 0;

	INT32 LOCAL_5 = g_pGuildAlly[LOCAL_3];
	if (LOCAL_5 <= 0 || LOCAL_5 >= LOCAL_2)
		LOCAL_5 = 0;

	if (LOCAL_3 != 0 && LOCAL_4 != 0 && g_pGuildWar[LOCAL_4] != LOCAL_3)
		LOCAL_1 = LOCAL_4;
	else
		LOCAL_1 = 0;

	if (false)
	{
		//SendSignalParm2(arg1, 0x7530 ,0x3A8, LOCAL_1, arg2);
	}
	else
		SendSignalParm3(clientId, 0x7530, 0x3A8, LOCAL_1, capeWin, LOCAL_5);
}

void SendGuildList(INT32 clientId)
{
	INT32 LOCAL_1 = pMob[clientId].Mobs.Player.GuildIndex;

	if (LOCAL_1 <= 0)
		return;

	char LOCAL_65[108] = {};

	INT32 LOCAL_66 = 0;
	INT32 LOCAL_67 = 70;
	INT32 LOCAL_68 = 1;

	for (; LOCAL_68 < MAX_PLAYER; LOCAL_68++)
	{
		if (pUser[LOCAL_68].Status == USER_PLAY && pMob[LOCAL_68].Mobs.Player.GuildIndex == LOCAL_1)
		{
			LOCAL_66++;

			INT32 LOCAL_69 = strlen(LOCAL_65);

			if (LOCAL_69 >= LOCAL_67)
			{
				SendClientMessage(clientId, LOCAL_65);

				LOCAL_65[0] = 0;
			}

			pMob[LOCAL_68].Mobs.Player.Name[15] = 0;
			pMob[LOCAL_68].Mobs.Player.Name[14] = 0;

			strcat_s(LOCAL_65, pMob[LOCAL_68].Mobs.Player.Name);
			strcat_s(LOCAL_65, " ");
		}
	}

	if (LOCAL_65[0])
		SendClientMessage(clientId, LOCAL_65);

	if (LOCAL_66 == 0)
		SendClientMessage(clientId, g_pLanguageString[_NN_No_Guild_Members]);

	INT32 LOCAL_70 = MAX_GUILD;

	if (LOCAL_1 > 0 && LOCAL_1 < LOCAL_70)
	{
		INT32 LOCAL_71 = g_pGuildWar[LOCAL_1],
			LOCAL_80 = -1;

		if (LOCAL_71 > 0 && LOCAL_71 < LOCAL_70)
		{
			INT32 LOCAL_81 = g_pGuildWar[LOCAL_71];

			if (LOCAL_1 == LOCAL_81)
			{
				SendClientMessage(clientId, g_pLanguageString[_SN_Your_are_at_war], g_pGuild[LOCAL_71].Name.c_str());

				LOCAL_80 = LOCAL_71;
			}
			else
				SendClientMessage(clientId, g_pLanguageString[_SN_War_to_S], g_pGuild[LOCAL_71].Name.c_str());
		}

		INT32 LOCAL_82 = 1;
		for (; LOCAL_82 < LOCAL_70; LOCAL_82++)
		{
			if (g_pGuildWar[LOCAL_82] == LOCAL_1 && LOCAL_82 != LOCAL_80)
				SendClientMessage(clientId, g_pLanguageString[_SN_War_from_S], g_pGuild[LOCAL_82].Name.c_str());
		}

		LOCAL_71 = g_pGuildAlly[LOCAL_1];
		if (LOCAL_71 > 0 && LOCAL_71 < LOCAL_70)
			SendClientMessage(clientId, g_pLanguageString[_SN_Ally_to_S], g_pGuild[LOCAL_71].Name.c_str());

		LOCAL_82 = 1;
		for (; LOCAL_82 < LOCAL_70; LOCAL_82++)
		{
			if (g_pGuildAlly[LOCAL_82] == LOCAL_1)
				SendClientMessage(clientId, g_pLanguageString[_SN_Ally_from_S], g_pGuild[LOCAL_82].Name.c_str());
		}
	}
}

void SendWeather()
{
	p18B packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x18B;
	packet.Header.Size = sizeof packet;
	packet.Header.ClientId = 0x7530;

	packet.WeatherId = sServer.Weather;

	for (INT32 i = 1; i < MAX_PLAYER; i++)
	{
		if (pUser[i].Status != USER_PLAY)
			continue;

		INT32 LOCAL_7;
		if (pMob[i].Target.X >> 7 < 12 && pMob[i].Target.Y >> 7 > 25)
			LOCAL_7 = 1;
		else
			LOCAL_7 = 0;

		if (LOCAL_7 != 0)
			continue;

		pUser[i].AddMessage((BYTE*)&packet, sizeof packet);
	}
}

void SendChatGuild(INT32 clientId, INT32 guildId, const char *msg, ...)
{
	if (clientId <= 0 || clientId >= MAX_PLAYER)
		return;

	/* Arglist */
	char buffer[128];
	va_list arglist;
	va_start(arglist, msg);
	vsprintf_s(buffer, msg, arglist);
	va_end(arglist);

	p334 packet;
	memset(&packet, 0, sizeof packet);

	packet.Header.PacketId = 0x334;
	packet.Header.Size = sizeof p334;
	packet.Header.ClientId = SERVER_SIDE;

	sprintf_s(packet.eValue, buffer);
	strncpy_s(packet.eCommand, g_pGuild[guildId].Name.c_str(), 16);

	char *p = (char*)&packet;
	*(INT16*)&p[124] = 3;

	pUser[clientId].AddMessage((BYTE*)&packet, sizeof p334);
}

void SendRepurchase(int clientId)
{
	p3E8 packet{};

	INT32 cityZone = GetVillage(pMob[clientId].Target.X, pMob[clientId].Target.Y);
	if (cityZone == 5)
		cityZone = 4;

	for (int i = 0; i < 10; i++)
	{
		packet.Item[i].index = i + 1;

		STRUCT_ITEM* item = &pUser[clientId].Repurchase.Items[i];
		if (item->Index <= 0 || item->Index >= MAX_ITEMLIST)
			continue;

		int price, impost;
		std::tie(price, impost) = GetPriceAndImpost(&ItemList[item->Index], cityZone);

		packet.Item[i].item = *item;
		packet.Item[i].sellPrice = price;
	}

	packet.Header.PacketId = 0x3E8;
	packet.Header.Size = sizeof p3E8;

	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof p3E8);
}

void SendQuiz(int clientId)
{
	MSG_QUIZ packet{};
	packet.Header.PacketId = 0x1C6;
	packet.Header.Size = sizeof MSG_QUIZ;
	packet.Header.ClientId = clientId;

	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof MSG_QUIZ);
}

void SendQuiz(int clientId, const char* question, std::array<std::string, 4> answers)
{
	MSG_QUIZ packet{};
	packet.Header.PacketId = 0x1C6;
	packet.Header.Size = sizeof MSG_QUIZ;
	packet.Header.ClientId = clientId;

	strncpy_s(packet.Title, question, 95);

	for (int i = 0; i < 4; i++)
		strncpy_s(packet.Ans[i], answers[i].c_str(), 31);

	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof MSG_QUIZ);
}
 
void SendAutoPartyInfo(int clientId)
{
	MSG_AUTOPARTY packet{};
	packet.Header.PacketId = AutoPartyInfoPacket;
	packet.Header.ClientId = clientId;
	packet.Header.Size = sizeof packet;

	int i = 0;
	for (const auto& name : pUser[clientId].AutoParty.Nicknames)
	{
		if (name.empty())
			continue;

		strncpy_s(packet.Nickname[i++],  name.c_str(), 16);
	}

	packet.EnableAll = pUser[clientId].AutoParty.EnableAll;

	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof packet);
}
 
void MulticastGetCreateMob(int clientId)
{
	p364 packet{};

	GetCreateMob(clientId, (BYTE*)&packet);
	GridMulticast_2(pMob[clientId].Target.X, pMob[clientId].Target.Y, (BYTE*)&packet, 0);
}

void SendChatMessage(int clientId, int color, const char* message, ...)
{
	/* Arglist */
	char buffer[108];
	va_list arglist;
	va_start(arglist, message);
	vsprintf_s(buffer, message, arglist);
	va_end(arglist);
	/* Fim arlist */

	MSG_CHATMESSAGE packet{};
	packet.Header.PacketId = ChatMessagePacket;
	packet.Header.Size = sizeof packet;

	strncpy_s(packet.Message, buffer, 96);

	packet.Color = color;
	pUser[clientId].AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof packet);
}

void SendChatMessage(int color, const char* message, ...)
{
	/* Arglist */
	char buffer[108];
	va_list arglist;
	va_start(arglist, message);
	vsprintf_s(buffer, message, arglist);
	va_end(arglist);
	/* Fim arlist */

	MSG_CHATMESSAGE packet{};
	packet.Header.PacketId = ChatMessagePacket;
	packet.Header.Size = sizeof packet;

	strncpy_s(packet.Message, buffer, 96);

	packet.Color = color;

	for (auto& user : pUser)
	{
		if (user.Status == USER_PLAY)
			user.AddMessage(reinterpret_cast<BYTE*>(&packet), sizeof packet);
	}
}

 