#ifndef APICONFIG_H
#define APICONFIG_H

#include <QString>

// ── Groq API — shared across all AI modules ───────────────────
static const QString GROQ_API_KEY   = "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";
static const QString GROQ_API_URL   = "https://api.groq.com/openai/v1/chat/completions";
static const QString GROQ_API_MODEL = "llama-3.1-8b-instant";
static const QString GROQ_STT_API_URL   = "https://api.groq.com/openai/v1/audio/transcriptions";
static const QString GROQ_STT_API_MODEL = "whisper-large-v3-turbo";

#endif // APICONFIG_H
