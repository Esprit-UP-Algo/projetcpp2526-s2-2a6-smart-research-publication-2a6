"""
SmartVision Voice Command Server
Lance ce script une fois : python voiceserver.py
Écoute sur http://127.0.0.1:5001/parse
"""

from flask import Flask, request, jsonify
import unicodedata
import re

app = Flask(__name__)

# ── Normalisation (supprime accents, met en minuscules) ───────────
def norm(s: str) -> str:
    return unicodedata.normalize('NFD', s.lower()).encode('ascii', 'ignore').decode('ascii')

# ── Modules et leurs mots-clés ────────────────────────────────────
MODULES = {
    'biosample':   ['biosample', 'bio sample', 'biologie', 'echantillon', 'prelevement', 'tube', 'ech'],
    'employee':    ['employe', 'personnel', 'collaborateur', 'membre', 'staff', 'agent', 'employes'],
    'equipement':  ['equipement', 'materiel', 'instrument', 'appareil', 'dispositif', 'outil', 'machine'],
    'experience':  ['experience', 'manip', 'manipulation', 'protocole', 'essai', 'test', 'experiences'],
    'projet':      ['projet', 'programme', 'initiative', 'mission', 'dossier', 'projets'],
    'publication': ['publication', 'article', 'rapport', 'these', 'papier', 'recherche', 'publications'],
}

# ── Actions et leurs mots-clés ────────────────────────────────────
ACTIONS = {
    'congelateur': ['congelateur', 'frigo', 'freezer', 'cong', 'ia cong', 'ai cong'],
    'chatbot':     ['chatbot', 'chat bot', 'assistant', 'bot', 'aide ia', 'parler ia'],
    'logout':      ['deconnecter', 'deconnexion', 'sortir', 'quitter', 'logout', 'sign out', 'deloger'],
    'add':         ['ajoute', 'ajouter', 'nouveau', 'nouvelle', 'creer', 'cree', 'inserer', 'saisir', 'new', 'add', 'formulaire ajout'],
    'edit':        ['modifie', 'modifier', 'edite', 'editer', 'change', 'mettre a jour', 'update', 'corriger', 'rectifier'],
    'delete':      ['supprime', 'supprimer', 'efface', 'effacer', 'enleve', 'enlever', 'retirer', 'virer', 'eliminer', 'delete'],
    'details':     ['details', 'infos', 'informations', 'voir plus', 'consulter', 'fiche', 'descriptif'],
    'stats':       ['stats', 'statistiques', 'graphique', 'graphe', 'dashboard', 'tableau de bord', 'analyse', 'bilan'],
    'export_pdf':  ['export', 'exporte', 'pdf', 'imprimer', 'telecharger', 'generer pdf', 'rapport pdf'],
    'search':      ['recherche', 'cherche', 'filtre', 'trouver', 'trouve', 'chercher', 'filtrer', 'scanner'],
    'refresh':     ['actualise', 'rafraichir', 'recharger', 'reload', 'actualiser', 'recharge'],
    'save':        ['enregistre', 'sauvegarder', 'valider', 'valide', 'sauver', 'confirmer', 'terminer', 'ok'],
    'cancel':      ['annule', 'annuler', 'retour', 'fermer', 'abandonner', 'stopper'],
    'navigate':    ['ouvre', 'ouvrir', 'affiche', 'va dans', 'montre', 'accede', 'aller', 'voir', 'visualise', 'selectionne', 'module'],
}

# Priorité des actions (les premières sont vérifiées en premier)
ACTION_ORDER = ['congelateur', 'chatbot', 'logout', 'add', 'edit', 'delete',
                'details', 'stats', 'export_pdf', 'search', 'refresh', 'save', 'cancel', 'navigate']

# ── Descriptions lisibles ─────────────────────────────────────────
DESCRIPTIONS = {
    ('navigate',   'biosample'):   'Ouvrir BioSample',
    ('navigate',   'employee'):    'Ouvrir Employés',
    ('navigate',   'equipement'):  'Ouvrir Équipements',
    ('navigate',   'experience'):  'Ouvrir Expériences',
    ('navigate',   'projet'):      'Ouvrir Projets',
    ('navigate',   'publication'): 'Ouvrir Publications',
    ('add',        'biosample'):   'Ajouter un échantillon',
    ('add',        'employee'):    'Ajouter un employé',
    ('add',        'equipement'):  'Ajouter un équipement',
    ('add',        'experience'):  'Ajouter une expérience',
    ('add',        'projet'):      'Ajouter un projet',
    ('add',        'publication'): 'Ajouter une publication',
    ('edit',       'biosample'):   'Modifier BioSample',
    ('edit',       'employee'):    'Modifier un employé',
    ('edit',       'equipement'):  'Modifier un équipement',
    ('edit',       'experience'):  'Modifier une expérience',
    ('edit',       'projet'):      'Modifier un projet',
    ('edit',       'publication'): 'Modifier une publication',
    ('delete',     'biosample'):   'Supprimer un échantillon',
    ('delete',     'employee'):    'Supprimer un employé',
    ('delete',     'equipement'):  'Supprimer un équipement',
    ('delete',     'experience'):  'Supprimer une expérience',
    ('delete',     'projet'):      'Supprimer un projet',
    ('delete',     'publication'): 'Supprimer une publication',
    ('details',    'experience'):  'Détails expérience',
    ('details',    'equipement'):  'Détails équipement',
    ('details',    'projet'):      'Détails projet',
    ('details',    'publication'): 'Détails publication',
    ('stats',      'biosample'):   'Statistiques BioSample',
    ('stats',      'employee'):    'Statistiques Employés',
    ('stats',      'experience'):  'Statistiques Expériences',
    ('stats',      'publication'): 'Statistiques Publications',
    ('export_pdf', 'biosample'):   'Exporter PDF BioSample',
    ('refresh',    ''):            'Actualiser',
    ('save',       ''):            'Enregistrer',
    ('cancel',     ''):            'Annuler',
    ('congelateur',''):            'Ouvrir Congélateur IA',
    ('chatbot',    ''):            'Ouvrir Chatbot IA',
    ('logout',     ''):            'Déconnexion',
}

# ── Parsing ───────────────────────────────────────────────────────
def parse_command(text: str, context: str = '') -> dict:
    t = norm(text)

    # 1. Detect action (priority order)
    detected_action = ''
    for action in ACTION_ORDER:
        for kw in ACTIONS[action]:
            # word-boundary-aware matching
            if re.search(r'(?<!\w)' + re.escape(kw) + r'(?!\w)', t):
                detected_action = action
                break
        if detected_action:
            break

    # Default to navigate if nothing found
    if not detected_action:
        detected_action = 'navigate'

    # 2. Global actions have no module
    if detected_action in ('congelateur', 'chatbot', 'logout', 'save', 'cancel'):
        desc = DESCRIPTIONS.get((detected_action, ''), detected_action.capitalize())
        return {'action': detected_action, 'module': '', 'description': desc, 'fields': {}, 'text': ''}

    # 3. Detect module
    detected_module = ''
    for module, keywords in MODULES.items():
        for kw in keywords:
            if re.search(r'(?<!\w)' + re.escape(kw) + r'(?!\w)', t):
                detected_module = module
                break
        if detected_module:
            break

    # Use context as fallback when no module found in text
    if not detected_module and context:
        detected_module = context

    # 4. Extract search text for 'search' action
    search_text = ''
    if detected_action == 'search':
        for kw in ACTIONS['search']:
            m = re.search(r'(?<!\w)' + re.escape(kw) + r'(?!\w)\s*(.*)', t)
            if m:
                search_text = m.group(1).strip()
                break

    # 5. Build description
    desc = DESCRIPTIONS.get((detected_action, detected_module), '')
    if not desc:
        mod_label = detected_module.capitalize() if detected_module else ''
        act_label = detected_action.replace('_', ' ').capitalize()
        desc = f'{act_label} {mod_label}'.strip()

    return {
        'action':      detected_action,
        'module':      detected_module,
        'description': desc,
        'fields':      {},
        'text':        search_text,
    }

# ── Routes Flask ──────────────────────────────────────────────────
@app.route('/parse', methods=['POST'])
def parse_route():
    data    = request.get_json(force=True, silent=True) or {}
    text    = data.get('text', '').strip()
    context = data.get('context', '').strip()
    if not text:
        return jsonify({'action': 'unknown', 'module': '', 'description': 'Texte vide', 'fields': {}, 'text': ''}), 400
    result = parse_command(text, context)
    return jsonify(result)

@app.route('/health', methods=['GET'])
def health():
    return jsonify({'status': 'ok', 'service': 'SmartVision Voice Server'})

if __name__ == '__main__':
    print("=== SmartVision Voice Command Server ===")
    print("Écoute sur http://127.0.0.1:5001/parse")
    print("Arrêter avec Ctrl+C")
    app.run(host='127.0.0.1', port=5001, debug=False, threaded=True)
