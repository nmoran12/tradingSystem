export type Trade = {
  price: number;
  quantity: number;
  aggressiveOrderId?: number;
  restingOrderId?: number;
};

export type Level = {
  price: number;
  quantity: number;
};

export type ReplayStep = {
  schemaVersion?: number;
  index: number;
  commandType: string;
  side: string;
  orderType: string;
  orderId: number;
  price: number;
  quantity: number;
  symbol?: string;
  bestBid: number | null;
  bestAsk: number | null;
  spread: number | null;
  restingBidLevels: Level[];
  restingAskLevels: Level[];
  trades: Trade[];
  totalRestingOrders: number;
  totalRestingQuantity: number;
};

export type Source =
  | { kind: 'none' }
  | { kind: 'scenario'; label: string; file: string }
  | { kind: 'file'; name: string }
  | { kind: 'live'; url: string };
