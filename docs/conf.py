project = 'glnext'
copyright = '2021, Dombi Szabolcs'
author = 'Dombi Szabolcs'

release = '0.8.1'

extensions = [
    'sphinx_rtd_theme',
]

templates_path = []

exclude_patterns = []

html_theme = 'sphinx_rtd_theme'

html_static_path = ['static']


def setup(app):
    app.add_stylesheet('css/custom.css')
