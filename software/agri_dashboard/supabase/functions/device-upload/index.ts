import { createClient } from "npm:@supabase/supabase-js@2";

type RiskLevel = "LOW" | "MEDIUM" | "HIGH" | "CRITICAL";
type DbRiskLevel = "LOW" | "MEDIUM" | "MEDIUM_HIGH" | "HIGH" | "UNKNOWN";
type RelayAction = "ON" | "OFF" | "NONE" | "REQUEST";
type LiquidLevel = "OK" | "LOW" | "UNKNOWN";
type Alarm =
  | "NONE"
  | "LOW_LIQUID"
  | "CURRENT_ABNORMAL"
  | "LOW_CONFIDENCE"
  | "SENSOR_ERROR"
  | "UNKNOWN";

type DiseaseKnowledge = {
  disease: string;
  disease_cn: string;
  risk_level: DbRiskLevel;
  pesticide: string;
  suggestion: string;
  pump_action: "ON" | "OFF";
  fan_action: "ON" | "OFF";
  pump_duration_s: number;
  fan_duration_s: number;
};

const DEVICE_IDS = ["Node01", "Node02", "Node03"] as const;
const RISK_VALUES: RiskLevel[] = ["LOW", "MEDIUM", "HIGH", "CRITICAL"];
const RELAY_VALUES: RelayAction[] = ["ON", "OFF", "NONE", "REQUEST"];
const LIQUID_VALUES: LiquidLevel[] = ["OK", "LOW", "UNKNOWN"];
const ALARM_VALUES: Alarm[] = [
  "NONE",
  "LOW_LIQUID",
  "CURRENT_ABNORMAL",
  "LOW_CONFIDENCE",
  "SENSOR_ERROR",
  "UNKNOWN",
];
const MAX_CLOCK_SKEW_MS = 30 * 60 * 1000;

const DISEASES: Record<number, DiseaseKnowledge> = {
  0: {
    disease: "Pepper_Bell_Bacterial_Spot",
    disease_cn: "甜椒细菌性斑点病",
    risk_level: "MEDIUM",
    pesticide: "铜制剂、春雷霉素或其他登记细菌性病害防治药剂方向，需人工确认后按当地农技规范使用。",
    suggestion: "建议清除重病叶，降低叶面湿度，避免喷淋飞溅，并加强通风；病情扩展时人工复核后规范用药。",
    pump_action: "OFF",
    fan_action: "ON",
    pump_duration_s: 0,
    fan_duration_s: 8,
  },
  1: {
    disease: "Pepper_Bell_Healthy",
    disease_cn: "甜椒健康",
    risk_level: "LOW",
    pesticide: "无需用药。",
    suggestion: "保持常规巡检，维持适宜通风和水肥管理。",
    pump_action: "OFF",
    fan_action: "OFF",
    pump_duration_s: 0,
    fan_duration_s: 0,
  },
  2: {
    disease: "Potato_Early_Blight",
    disease_cn: "马铃薯早疫病",
    risk_level: "MEDIUM_HIGH",
    pesticide: "代森锰锌、百菌清、嘧菌酯或吡唑醚菌酯等防治方向，需人工确认后按当地农技规范使用。",
    suggestion: "建议及时处理病叶，保护功能叶片，保持水分均衡；病情扩展时人工复核后规范用药。",
    pump_action: "ON",
    fan_action: "ON",
    pump_duration_s: 5,
    fan_duration_s: 8,
  },
  3: {
    disease: "Potato_Late_Blight",
    disease_cn: "马铃薯晚疫病",
    risk_level: "HIGH",
    pesticide: "甲霜灵/精甲霜灵与保护剂混用，或霜脲氰、烯酰吗啉等方向，需人工确认后使用。",
    suggestion: "晚疫病风险高，建议立即人工复核并处理，降低叶面湿度，防止快速扩散。",
    pump_action: "ON",
    fan_action: "ON",
    pump_duration_s: 5,
    fan_duration_s: 10,
  },
  4: {
    disease: "Potato_Healthy",
    disease_cn: "马铃薯健康",
    risk_level: "LOW",
    pesticide: "无需用药。",
    suggestion: "当前识别为健康马铃薯，保持常规巡检和水分管理。",
    pump_action: "OFF",
    fan_action: "OFF",
    pump_duration_s: 0,
    fan_duration_s: 0,
  },
  5: {
    disease: "Tomato_Early_Blight",
    disease_cn: "番茄早疫病",
    risk_level: "MEDIUM_HIGH",
    pesticide: "百菌清、代森锰锌、铜制剂或嘧菌酯类保护性杀菌剂方向，需人工确认后轮换使用。",
    suggestion: "建议清除病叶，控制湿度，改善通风，病情扩展时人工复核后规范用药。",
    pump_action: "ON",
    fan_action: "ON",
    pump_duration_s: 5,
    fan_duration_s: 8,
  },
  6: {
    disease: "Tomato_Late_Blight",
    disease_cn: "番茄晚疫病",
    risk_level: "HIGH",
    pesticide: "代森锰锌、百菌清、霜脲氰、烯酰吗啉等防治方向，需人工确认后按当地农技规范轮换使用。",
    suggestion: "番茄晚疫病为高风险病害，建议立即人工复核并处理，及时喷药并加强通风降湿。",
    pump_action: "ON",
    fan_action: "ON",
    pump_duration_s: 5,
    fan_duration_s: 10,
  },
  7: {
    disease: "Tomato_Healthy",
    disease_cn: "番茄健康",
    risk_level: "LOW",
    pesticide: "无需用药。",
    suggestion: "当前识别为健康番茄，不建议自动喷药，仅保持监测。",
    pump_action: "OFF",
    fan_action: "OFF",
    pump_duration_s: 0,
    fan_duration_s: 0,
  },
};

function jsonResponse(body: Record<string, unknown>, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json; charset=utf-8" },
  });
}

function fail(error: string, status = 400): Response {
  return jsonResponse({ ok: false, error }, status);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isFiniteNumber(value: unknown): value is number {
  return typeof value === "number" && Number.isFinite(value);
}

function optionalNumber(value: unknown): number | null {
  if (value === undefined || value === null || value === "") return null;
  return isFiniteNumber(value) ? value : NaN;
}

function normalizeRelayForDb(value: RelayAction): RelayAction {
  return RELAY_VALUES.includes(value) ? value : "OFF";
}

function optionalStatus(value: unknown): string {
  if (value === undefined || value === null || value === "") return "UNKNOWN";
  return String(value).trim().slice(0, 64) || "UNKNOWN";
}

function secretNameFor(prefix: string, deviceId: string): string {
  return `${prefix}_${deviceId.toUpperCase()}`;
}

function bytesToHex(bytes: ArrayBuffer): string {
  return [...new Uint8Array(bytes)].map((byte) => byte.toString(16).padStart(2, "0")).join("");
}

function safeEqual(a: string, b: string): boolean {
  if (a.length !== b.length) return false;
  let diff = 0;
  for (let index = 0; index < a.length; index += 1) {
    diff |= a.charCodeAt(index) ^ b.charCodeAt(index);
  }
  return diff === 0;
}

async function hmacSha256Hex(secret: string, body: string): Promise<string> {
  const encoder = new TextEncoder();
  const key = await crypto.subtle.importKey(
    "raw",
    encoder.encode(secret),
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"],
  );
  const signature = await crypto.subtle.sign("HMAC", key, encoder.encode(body));
  return bytesToHex(signature);
}

function parseJson(rawBody: string): Record<string, unknown> | Response {
  try {
    const data: unknown = JSON.parse(rawBody);
    if (!isRecord(data)) return fail("JSON body must be an object");
    return data;
  } catch {
    return fail("invalid JSON body");
  }
}

function validateTimestamp(value: unknown): string | null {
  if (typeof value !== "string" || !value.trim()) return "timestamp is required";
  const timestamp = Date.parse(value);
  if (!Number.isFinite(timestamp)) return "timestamp must be ISO 8601";
  if (Math.abs(Date.now() - timestamp) > MAX_CLOCK_SKEW_MS) {
    return "timestamp is expired or too far in the future";
  }
  return null;
}

async function authenticate(req: Request, rawBody: string, data: Record<string, unknown>): Promise<string | null> {
  const bodyDeviceId = String(data.device_id ?? "");
  const headerDeviceId = req.headers.get("x-device-id") ?? "";
  if (!headerDeviceId) return "missing X-Device-Id";
  if (headerDeviceId !== bodyDeviceId) return "X-Device-Id does not match body device_id";

  const token = req.headers.get("x-device-token") ?? "";
  if (!token) return "missing X-Device-Token";
  const expectedToken = Deno.env.get(secretNameFor("DEVICE_TOKEN", bodyDeviceId)) ?? "";
  if (!expectedToken) return `missing token secret for ${bodyDeviceId}`;
  if (!safeEqual(token, expectedToken)) return "invalid X-Device-Token";

  const headerTimestamp = req.headers.get("x-timestamp") ?? "";
  if (!headerTimestamp) return "missing X-Timestamp";
  const headerTimestampError = validateTimestamp(headerTimestamp);
  if (headerTimestampError) return `X-Timestamp ${headerTimestampError}`;

  const requireHmac = (Deno.env.get("REQUIRE_HMAC") ?? "true").toLowerCase() !== "false";
  if (!requireHmac) return null;

  const providedSignature = (
    req.headers.get("x-signature") ??
    req.headers.get("x-device-signature") ??
    ""
  ).toLowerCase();
  if (!providedSignature) return "missing X-Signature";
  if (!/^[0-9a-f]{64}$/.test(providedSignature)) return "invalid X-Signature format";

  const hmacSecret = Deno.env.get(secretNameFor("DEVICE_HMAC_SECRET", bodyDeviceId)) ?? "";
  if (!hmacSecret) return `missing HMAC secret for ${bodyDeviceId}`;
  const expectedSignature = await hmacSha256Hex(hmacSecret, rawBody);
  if (!safeEqual(providedSignature, expectedSignature)) return "invalid X-Signature";
  return null;
}

function validatePayload(data: Record<string, unknown>): string | null {
  if (data.schema_version !== "1.0") return 'schema_version must be "1.0"';

  const deviceId = String(data.device_id ?? "");
  if (!DEVICE_IDS.includes(deviceId as typeof DEVICE_IDS[number])) {
    return "device_id must be Node01, Node02, or Node03";
  }

  const timestampError = validateTimestamp(data.timestamp);
  if (timestampError) return timestampError;

  if (!Number.isInteger(data.disease_id) || Number(data.disease_id) < 0 || Number(data.disease_id) > 7) {
    return "disease_id must be an integer from 0 to 7";
  }

  if (!isFiniteNumber(data.confidence) || data.confidence < 0 || data.confidence > 1) {
    return "confidence must be a number from 0 to 1";
  }

  if (!RISK_VALUES.includes(String(data.risk_level) as RiskLevel)) {
    return "risk_level must be LOW, MEDIUM, HIGH, or CRITICAL";
  }

  if (!RELAY_VALUES.includes(String(data.pump_action) as RelayAction)) {
    return "pump_action must be ON, OFF, NONE, or REQUEST";
  }

  if (!RELAY_VALUES.includes(String(data.fan_action) as RelayAction)) {
    return "fan_action must be ON, OFF, NONE, or REQUEST";
  }

  if (!LIQUID_VALUES.includes(String(data.liquid_level) as LiquidLevel)) {
    return "liquid_level must be OK, LOW, or UNKNOWN";
  }

  if (!ALARM_VALUES.includes(String(data.alarm) as Alarm)) {
    return "alarm must be NONE, LOW_LIQUID, CURRENT_ABNORMAL, LOW_CONFIDENCE, SENSOR_ERROR, or UNKNOWN";
  }

  for (const key of [
    "temperature_c",
    "humidity_percent",
    "light_lux",
    "soil_moisture_percent",
    "current_ma",
  ]) {
    const value = optionalNumber(data[key]);
    if (Number.isNaN(value)) return `${key} must be a number when provided`;
  }

  for (const key of ["pump_duration_s", "fan_duration_s", "uptime_ms"]) {
    const value = optionalNumber(data[key]);
    if (Number.isNaN(value) || (value !== null && value < 0)) {
      return `${key} must be a non-negative number when provided`;
    }
  }

  return null;
}

function normalizeDbRisk(deviceRisk: RiskLevel, expertRisk: DbRiskLevel): DbRiskLevel {
  if (deviceRisk === "CRITICAL") return "HIGH";
  return expertRisk;
}

function buildDbPayload(data: Record<string, unknown>): Record<string, unknown> {
  const diseaseId = Number(data.disease_id);
  const confidence = Number(data.confidence);
  const expert = DISEASES[diseaseId];
  const deviceRisk = String(data.risk_level) as RiskLevel;
  const liquidLevel = String(data.liquid_level) as LiquidLevel;
  const inputAlarm = String(data.alarm) as Alarm;
  const currentMa = optionalNumber(data.current_ma);

  let riskLevel = normalizeDbRisk(deviceRisk, expert.risk_level);
  let pumpAction = normalizeRelayForDb((data.pump_action ?? expert.pump_action) as RelayAction);
  let fanAction = normalizeRelayForDb((data.fan_action ?? expert.fan_action) as RelayAction);
  let pumpDuration = Number(data.pump_duration_s ?? expert.pump_duration_s);
  let fanDuration = Number(data.fan_duration_s ?? expert.fan_duration_s);
  let alarm: Alarm = inputAlarm;
  let suggestion = expert.suggestion;

  if (confidence < 0.6) {
    riskLevel = "UNKNOWN";
    pumpAction = "OFF";
    fanAction = "OFF";
    pumpDuration = 0;
    fanDuration = 0;
    alarm = "LOW_CONFIDENCE";
    suggestion = `${suggestion} 置信度低于 0.6，建议人工复核或重新采集图像，禁止自动喷药。`;
  }

  if (liquidLevel === "LOW") {
    pumpAction = "OFF";
    pumpDuration = 0;
    alarm = "LOW_LIQUID";
    suggestion = `${suggestion} 药液液位低，已禁止喷药，请补液后人工复核。`;
  }

  if (inputAlarm === "CURRENT_ABNORMAL" || (currentMa !== null && currentMa > 1200)) {
    pumpAction = "OFF";
    fanAction = "OFF";
    pumpDuration = 0;
    fanDuration = 0;
    alarm = "CURRENT_ABNORMAL";
    suggestion = `${suggestion} 检测到电流异常，已禁止执行继电器动作，请检查泵、风扇和供电回路。`;
  }

  return {
    schema_version: "1.0",
    timestamp: data.timestamp,
    device_id: data.device_id,
    disease_id: String(diseaseId),
    disease_class_id: diseaseId,
    disease: expert.disease,
    disease_cn: expert.disease_cn,
    confidence,
    risk_level: riskLevel,
    pesticide: expert.pesticide,
    suggestion,
    pump_action: pumpAction,
    fan_action: fanAction,
    pump_duration_s: pumpDuration,
    fan_duration_s: fanDuration,
    action_duration_s: pumpDuration,
    temperature_c: optionalNumber(data.temperature_c),
    humidity_percent: optionalNumber(data.humidity_percent),
    light_lux: optionalNumber(data.light_lux),
    soil_moisture_percent: optionalNumber(data.soil_moisture_percent),
    temperature: optionalNumber(data.temperature_c),
    humidity: optionalNumber(data.humidity_percent),
    light: optionalNumber(data.light_lux),
    soil_moisture: optionalNumber(data.soil_moisture_percent),
    liquid_level: liquidLevel,
    current_ma: currentMa,
    system_status: optionalStatus(data.system_status),
    sensor_status: optionalStatus(data.sensor_status),
    alarm,
    inference_source: "edge_function_l160_http",
    model_name: "stm32n647_device_report_v1",
    image_name: "device_json_v1",
    image_url: null,
  };
}

Deno.serve(async (req: Request) => {
  if (req.method !== "POST") {
    return fail("method not allowed; use POST", 405);
  }

  const contentType = req.headers.get("content-type") ?? "";
  if (!contentType.toLowerCase().includes("application/json")) {
    return fail("Content-Type must be application/json", 415);
  }

  const rawBody = await req.text();
  const dataOrResponse = parseJson(rawBody);
  if (dataOrResponse instanceof Response) return dataOrResponse;

  const validationError = validatePayload(dataOrResponse);
  if (validationError) return fail(validationError);

  const authError = await authenticate(req, rawBody, dataOrResponse);
  if (authError) return fail(authError, 401);

  const supabaseUrl = Deno.env.get("SUPABASE_URL");
  const serviceRoleKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY");
  if (!supabaseUrl || !serviceRoleKey) {
    return fail("missing SUPABASE_URL or SUPABASE_SERVICE_ROLE_KEY", 500);
  }

  const supabase = createClient(supabaseUrl, serviceRoleKey, {
    auth: { persistSession: false, autoRefreshToken: false },
  });

  const payload = buildDbPayload(dataOrResponse);
  const { data, error } = await supabase
    .from("diagnosis_records")
    .insert(payload)
    .select("id")
    .single();

  if (error) {
    return fail(error.message, 500);
  }

  return jsonResponse({
    ok: true,
    id: data?.id,
    message: "device record inserted by edge function",
  });
});
