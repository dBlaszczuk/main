import xml.etree.ElementTree as ET
import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# Kod zawiera funkcje pozwalające na manipulacje bazą danych leków z pliku XML
# W celu poprawnego działania w folderze z plikiem projekt.py musi znajdować się plik drugbank_partial.xml
# Czyli okrojona wersja bazy danych leków DrugBank aczkolwiek zawierająca wszystkie potrzebne dane
# analogiczne do pełnej wersji baz danych leków

def fun1(file_path):
    # Parsowanie pliku XML
    tree = ET.parse(file_path)
    root = tree.getroot()
    
    # Definiujemy przestrzeń nazw
    ns = {'db': 'http://www.drugbank.ca'}
    
    data = []
    
    # Iteracja po wszystkich elementach <drug>
    for drug in root.findall('db:drug', ns):
        # Pobranie unikalnego identyfikatora (primary="true") aby uniknąć duplikacji
        primary_id = None
        # Sprawdzenie wszystkich <drugbank_id> w <drug> i gdy znaleziono primary ustawienie
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        # Nazwa     
        name_el = drug.find('db:name', ns)
        name = name_el.text if drug.find('db:name', ns) is not None else None

        # Typ leku get bo ejst atrybutem
        drug_type = drug.get('type')

        # Opis leku lub none gdy brak
        description_el = drug.find('db:description', ns)
        description = description_el.text if drug.find('db:description', ns) is not None else None

        # Postać leku (pobrana z pierwszego produktu)
        product = drug.find('db:products/db:product', ns)
        dosage_form = product.find('db:dosage-form', ns).text if product is not None and product.find('db:dosage-form', ns) is not None else None

        # Wskazania
        indication = drug.find('db:indication', ns).text if drug.find('db:indication', ns) is not None else None

        # Mechanizm działania
        moa_elem = drug.find('db:mechanism-of-action', ns)
        moa = moa_elem.text if drug.find('db:mechanism-of-action', ns) is not None else None

        # Interakcje z pokarmami – element może nie występować
        food_interactions_elem = drug.find('db:food-interactions', ns)
        food_interactions = food_interactions_elem.text if food_interactions_elem is not None else None

        data.append({
            'drugbank_id': primary_id,
            'name': name,
            'type': drug_type,
            'description': description,
            'dosage_form': dosage_form,
            'indication': indication,
            'mechanism_of_action': moa,
            'food_interactions': food_interactions
        })
    
    # Tworzymy i zwracamy ramkę danych
    df = pd.DataFrame(data)
    return df

def fun2(file_path):
    # To samo co poprzednio tylko tworze df synonimow zamiast tego co ostatnio
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {'db': 'http://www.drugbank.ca'}

    data = []

    for drug in root.findall('db:drug', ns):
        primary_id = None
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        # Lista synonimów
        synonyms = [syn.text for syn in drug.findall('db:synonyms/db:synonym', ns) if syn.text]

        # Dodanie do listy
        for synonym in synonyms:
            data.append({'drugbank_id': primary_id, 'synonym': synonym})

    # Ramka
    df_synonyms = pd.DataFrame(data)

    for drug_id in df_synonyms['drugbank_id'].unique():
        draw_synonym_graph(drug_id, df_synonyms)

def draw_synonym_graph(drugbank_id, df_synonyms):
    # Pobranie synonimów z df
    synonyms = df_synonyms[df_synonyms['drugbank_id'] == drugbank_id]['synonym'].tolist()

    # Tworzenie grafu
    G = nx.Graph()
    
    # Dodanie węzłów i krawędzi
    G.add_node(drugbank_id, color='red')
    for synonym in synonyms:
        G.add_node(synonym, color='green') 
        G.add_edge(drugbank_id, synonym)

    # Rysowanie grafu
    plt.figure(figsize=(16, 16))
    colors = [G.nodes[n]['color'] for n in G.nodes]
    pos = nx.spring_layout(G, seed=420)
    nx.draw(G, pos, with_labels=True, node_color=colors, edge_color='gray', node_size=3000, font_size=12)
    
    plt.title(f"Synonym graph for {drugbank_id}")
    plt.show()

def fun3(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {'db': 'http://www.drugbank.ca'}

    data = []

    for drug in root.findall('db:drug', ns):
        primary_id = None
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        for product in drug.findall('db:products/db:product', ns):
            product_info = {
                'drugbank_id': primary_id,
                'product_name': product.findtext('db:name', 'No data', ns),
                'manufacturer': product.findtext('db:labeller', 'No data', ns),
                'ndc_code': product.findtext('db:ndc-product-code', 'No data', ns),
                'dosage_form': product.findtext('db:dosage-form', 'No data', ns),
                'route': product.findtext('db:route', 'No data', ns),
                'strength': product.findtext('db:strength', 'No data', ns),
                'country': product.findtext('db:country', 'No data', ns),
                'regulatory_agency': product.findtext('db:source', 'No data', ns)
            }
            data.append(product_info)

    df_products = pd.DataFrame(data)
    return df_products

def fun4(file_path, print_total=True):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {'db': 'http://www.drugbank.ca'}

    pathways_data = []

    for drug in root.findall('db:drug', ns):
        primary_id = None
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        for pathway in drug.findall('db:pathways/db:pathway', ns):
            pathway_info = {
                'drugbank_id': primary_id,
                'pathway_name': pathway.findtext('db:name', 'No data', ns),
                'pathway_category': pathway.findtext('db:category', 'No data', ns)
            }
            pathways_data.append(pathway_info)

    df_pathways = pd.DataFrame(pathways_data)
    if print_total:
        total_pathways = len(df_pathways)
        print(f"Total number of pathways: {total_pathways}")
    return df_pathways

def fun5(file_path):
    df_pathways = fun4(file_path, False)

    B = nx.Graph()
    for _, row in df_pathways.iterrows():
        B.add_node(row['drugbank_id'], bipartite=0, color='red')
        B.add_node(row['pathway_name'], bipartite=1, color='green')
        B.add_edge(row['drugbank_id'], row['pathway_name'])

    # Użycie układu bipartite_layout (graf dwudzielny)
    top_nodes = [n for n, d in B.nodes(data=True) if d['bipartite'] == 0]
    pos = nx.bipartite_layout(B, top_nodes)
    colors = [B.nodes[n]['color'] for n in B.nodes]

    plt.figure(figsize=(16, 16)) 
    nx.draw(B, pos, with_labels=True, node_color=colors, edge_color='black', node_size=800, font_size=12) 
    
    plt.title("Bipartite Graph of Drugs and Pathways")

    plt.figtext(0.5, 0.95, "Bipartite Graph of Drugs and Pathways", wrap=True, horizontalalignment='center', fontsize=12)
    
    plt.show()

def fun6(file_path):
    df_pathways = fun4(file_path, False)

    # Grupowanie danych według drugbank_id i liczenie liczby szlaków dla każdego leku
    pathway_counts = df_pathways.groupby('drugbank_id').size().reset_index(name='pathway_count')

    # Tworzenie histogramu
    plt.figure(figsize=(10, 6))
    plt.bar(pathway_counts['drugbank_id'], pathway_counts['pathway_count'], color='blue')
    plt.xlabel('DrugBank ID')
    plt.ylabel('Number of Pathways')
    plt.title('Number of Pathways Interacted by Each Drug')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.show()

    return pathway_counts

def fun7(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {"db": "http://www.drugbank.ca"}

    targets_data = []

    for drug in root.findall("db:drug", ns):
        primary_id = None
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        for target in drug.findall('db:targets/db:target', ns):
            target_id = target.findtext('db:id', 'No data', ns)
            polypep = target.find("db:polypeptide", ns)
            # Gdy polipeptyd istnieje
            if polypep is not None:
                external_db_id = polypep.attrib.get("id", "No data")
                source = polypep.attrib.get("source", "No data")
                poly_name = polypep.findtext("db:name", "No data", ns)
                gene_name = polypep.findtext("db:gene-name", "No data", ns)
                genatlas = "No data"
                ext_ids = polypep.find("db:external-identifiers", ns)
                if ext_ids is not None:
                    for ext_id in ext_ids.findall("db:external-identifier", ns):
                        res = ext_id.findtext("db:resource", "No data", ns)
                        if res == "GenAtlas":
                            genatlas = ext_id.findtext("db:identifier", "No data", ns)
                            break
                chromosome = polypep.findtext("db:chromosome-location", "No data", ns)
                cellular_location = polypep.findtext("db:cellular-location", "No data", ns)
            # Gdy polipeptyd nie istnieje
            else:
                external_db_id = "No data"
                source = "No data"
                poly_name = "No data"
                gene_name = "No data"
                genatlas = "No data"
                chromosome = "No data"
                cellular_location = "No data"

            #  Wsadzenie do ramki
            target_info = {
                "DrugBank ID Drug": primary_id,
                "Drug Name": drug.findtext("db:name", "No data", ns),
                "DrugBank ID Target": target_id,
                "Source": source,
                "External DB ID": external_db_id,
                "Polypeptide Name": poly_name,
                "Gene Name": gene_name,
                "GenAtlas ID": genatlas,
                "Chromosome": chromosome,
                "Cellular Location": cellular_location
            }
            targets_data.append(target_info)

    df_targets = pd.DataFrame(targets_data)
    return df_targets

def fun8(file_path):
    # Pobranie danych z funkcji fun7
    df_targets = fun7(file_path)

    # Grupowanie danych według "Cellular Location" i liczenie liczby wystąpień
    cellular_location_counts = df_targets['Cellular Location'].value_counts()

    # Tworzenie wykresu kołowego
    plt.figure(figsize=(14, 12))
    plt.pie(cellular_location_counts, labels=cellular_location_counts.index, autopct='%1.1f%%', startangle=200)
    plt.title('Procentowe występowanie targetów w różnych częściach komórki')
    plt.axis('equal')  # Upewnienie się, że wykres kołowy jest okrągły
    plt.show()

def fun9(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {"db": "http://www.drugbank.ca"}

    drug_status_data = {
        "approved": 0,
        "withdrawn": 0,
        "experimental/investigational": 0,
        "vet_approved": 0
    }

    approved_not_withdrawn = 0

    for drug in root.findall("db:drug", ns):
        groups_elem = drug.find("db:groups", ns)
        groups = []
        if groups_elem is not None:
            groups = [grp.text.strip().lower() for grp in groups_elem.findall("db:group", ns)]
        if "approved" in groups:
            drug_status_data["approved"] += 1
            if "withdrawn" not in groups:
                approved_not_withdrawn += 1
        if "withdrawn" in groups:
            drug_status_data["withdrawn"] += 1
        if "experimental" in groups or "investigational" in groups:
            drug_status_data["experimental/investigational"] += 1
        if "vet_approved" in groups:
            drug_status_data["vet_approved"] += 1

    df_drug_status = pd.DataFrame(list(drug_status_data.items()), columns=["Status", "Count"])

    plt.figure(figsize=(8, 8))
    plt.pie(df_drug_status["Count"], labels=df_drug_status["Status"], autopct='%1.1f%%', startangle=90)
    plt.title('Procentowe występowanie leków w różnych stanach')
    plt.axis('equal') 
    plt.show()

    print(f"Number of approved and not withdrawn drugs: {approved_not_withdrawn}")

    return df_drug_status

def fun10(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {"db": "http://www.drugbank.ca"}

    interactions_data = []

    for drug in root.findall("db:drug", ns):
        primary_id = None
        for drug_id in drug.findall('db:drugbank-id', ns):
            if drug_id.get('primary') == 'true':
                primary_id = drug_id.text
                break

        for interaction in drug.findall('db:drug-interactions/db:drug-interaction', ns):
            interaction_info = {
                "DrugBank ID Drug": primary_id,
                "Drug Name": drug.findtext("db:name", "No data", ns),
                "DrugBank ID Interaction": interaction.findtext('db:drugbank-id', 'No data', ns),
                "Interaction Name": interaction.findtext('db:name', 'No data', ns),
                "Description": interaction.findtext('db:description', 'No data', ns)
            }
            interactions_data.append(interaction_info)

    df_interactions = pd.DataFrame(interactions_data)
    return df_interactions

def fun11(file_path, gene_name_input=None):
    tree = ET.parse(file_path)
    root = tree.getroot()
    ns = {"db": "http://www.drugbank.ca"}

    drugs_interacting = set()
    drug_to_products = {}

    for drug in root.findall("db:drug", ns):
        name_elem = drug.find("db:name", ns)
        if name_elem is None:
            continue
        drug_name = name_elem.text.strip()
        targets_elem = drug.find("db:targets", ns)
        found = False
        if targets_elem is not None:
            for target in targets_elem.findall("db:target", ns):
                polypep = target.find("db:polypeptide", ns)
                if polypep is None:
                    continue
                gene_elem = polypep.find("db:gene-name", ns)
                if gene_elem is not None:
                    target_gene = gene_elem.text.strip().upper()
                    if target_gene == gene_name_input:
                        found = True
                        break
        if found:
            drugs_interacting.add(drug_name)
            products_elem = drug.find("db:products", ns)
            if products_elem is not None:
                for product in products_elem.findall("db:product", ns):
                    prod_name_elem = product.find("db:name", ns)
                    if prod_name_elem is not None:
                        prod_name = prod_name_elem.text.strip()
                        if drug_name in drug_to_products:
                            drug_to_products[drug_name].add(prod_name)
                        else:
                            drug_to_products[drug_name] = {prod_name}

    if not drugs_interacting:
        print(f"No drugs found containing gene '{gene_name_input}'.")
        return

    G = nx.Graph()
    G.add_node(gene_name_input, layer='gene')
    for drug in drugs_interacting:
        G.add_node(drug, layer='drug')
        G.add_edge(gene_name_input, drug)
    for drug, products in drug_to_products.items():
        if drug in drugs_interacting:
            for product in products:
                G.add_node(product, layer='product')
                G.add_edge(drug, product)

    pos = nx.spring_layout(G, seed=42)
    plt.figure(figsize=(12, 8))
    node_colors = []
    for node, attr in G.nodes(data=True):
        if attr['layer'] == 'gene':
            node_colors.append('purple')
        elif attr['layer'] == 'drug':
            node_colors.append('orange')
        elif attr['layer'] == 'product':
            node_colors.append('teal')

    nx.draw(G, pos, with_labels=True, node_color=node_colors, node_size=1500,
            font_size=12, font_weight='bold', edge_color='gray', width=2)
    plt.title(f"Interactions of gene {gene_name_input} with drugs and pharmaceutical products", fontsize=16)
    plt.axis("off")
    plt.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.05)
    gene_patch = mpatches.Patch(color='purple', label='Gene')
    drug_patch = mpatches.Patch(color='orange', label='Drug')
    product_patch = mpatches.Patch(color='teal', label='Product')
    plt.legend(handles=[gene_patch, drug_patch, product_patch], loc='upper left')
    plt.show()

# Wykres top 10 leków zawierających daną substancję leczniczą
def fun12(file_path):
    df_products = fun3(file_path)

    top_products = df_products['product_name'].value_counts().nlargest(10)

    # Tworzenie wykresu słupkowego
    plt.figure(figsize=(10, 6))
    top_products.plot(kind='bar', color='blue')
    plt.xlabel('Product Name')
    plt.ylabel('Count')
    plt.title('Top 10 Products Containing Each Drug')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    path = 'drugbank_partial.xml'
    #print(fun1(path))
    #fun2(path)
    #print(fun3(path))
    #print(fun4(path))
    #fun5(path)
    #fun6(path)
    #print(fun7(path))
    #fun8(path)
    #print(fun9(path))
    #print(fun10(path))
    #fun11(path, 'EGFR')
    fun12(path)
