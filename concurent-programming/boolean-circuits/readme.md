
Obwody współbieżne
Obwody boolowskie reprezentują wyrażenia boolowskie przy pomocy grafów, np. wyrażenie x∧(x∨¬y)∧(z∨y)
może być przedstawione za pomocą drzewa

lub, jeśli dopuścimy operatory dużych arności, za pomocą drzewa



Zwyczajowo obliczenia boolowskie są wyliczane od lewej do prawej, zatem w wyrażeniu x∧y najpierw wyliczamy wartość x a następnie wartość y. Tak zwane leniwe wyliczanie (lazy evaluation) może pominąć wyliczenie części podwyrażeń, jeśli wartość już obliczonych pozwala ustalić wartość całego wyrażenia. Na przykład wyrażenie true∨x nie musi wyliczać wartości x by poznać wartość całego wyrażenia, które wylicza się do true. Zwróćmy uwagę, że jeśli wyrażenia nie generują efektów ubocznych to kolejność wyliczania podwyrażeń nie powinna wpływać na wartość całego wyrażenia, więc wyliczanie wartości podwyrażeń może odbywać się współbieżnie.

Waszym zadaniem jest implementacja programu pozwalającego na współbieżne wyliczanie wartości wyrażeń boolowskich. Program powinien pozwalać na równoczesne wyliczanie wielu wyrażeń i wyliczać wartości pojedynczych wyrażeń współbieżnie.

Wyrażenie boolowskie
Wyrażenie boolowskie jest zdefiniowane indukcyjnie. Stałe true oraz false są wyrażeniami boolowskimi. NOT a, negacja wyrażenia boolowskiego a, jest wyrażeniem boolowskim. Koniunkcja AND(a1,a2,…) oraz alternatywa  OR(a1,a2,…) pewnej liczby wyrażeń boolowskich (co najmniej dwu) są wyrażeniami boolowskimi. Instrukcja warunkowa IF(a,b,c) jest wyrażeniem boolowskim. Również wyrażenia progowe GTx(a1,a2,…,an) oraz LTx(a1,a2,…,an), gdzie n≥1 i x≥0 są liczbami całkowitymi, są wyrażeniami boolowskimi.

Semantyka
Dla wyrażenia a, przez [a] oznaczamy wartość wyrażenia a.

        [true]=true

        [false]=false

        [AND(a1,a2,…,an)]=true

, jeśli każde wyrażenie ai, 1≤i≤n, spełnia [ai]=true, oraz [AND(a1,a2,…,an)]=false

    w przeciwnym przypadku.

        [OR(a1,a2,…,an)]=true

, jeśli istnieje wyrażenie ai, 1≤i≤n, spełnia [ai]=true, oraz [OR(a1,a2,…,an)]=false

    w przeciwnym przypadku.

        [GTx(a1,a2,…,an)]=true

, jeśli co najmniej x+1 wyrażeń ai, 1≤i≤n, spełnia [ai]=true, oraz [GTx(a1,a2,…,an)]=false

    w przeciwnym przypadku.

        [LTx(a1,a2,…,an)]=true

, jeśli co najwyżej x−1 wyrażeń ai, 1≤i≤n, spełnia [ai]=true, oraz LTx(a1,a2,…,an)]=false

    w przeciwnym przypadku.

        [IF(a,b,c)]=[b]

, jeśli [a]=true, oraz [IF(a,b,c)]=[c]

    w przeciwnym przypadku.

Specyfikacja

W rozwiązaniu obwody są reprezentowane poprzez obiekty klasy Circuit, a ich wartości wyliczane są poprzez obiekty implementujące interfejs CircuitSolver, zwane solverami.


public interface CircuitSolver {
    public CircuitValue solve(Circuit c);
    
    public void stop();
}

gdzie CircuitValue ma następujący interfejs.


public interface CircuitValue {
    public boolean getValue() throws InterruptedException;
}

Metoda solve(Circuit c) niezwłocznie zwraca specjalny obiekt typu CircuitValue reprezentujacy wartość obwodu. Wartość tę można pobrać wywołując metodę CircuitValue.getValue(), która czeka, aż wartość będzie wyliczona. Solvery powinny pozwalać na współbieżną obsługę wielu próśb (wywołań solve()) oraz o możliwie wpółbieżne obliczenie wartości obwodu.

Wywołanie metody stop() powinno spowodować zaprzestanie przyjmowania nowych zleceń solve() oraz niezwłocznie zakończyć wszystkie obecnie trwające obliczenia tego solvera. Od tego momentu, obiekty CircuitValue będące wynikiem nowych i przerwanych obliczeń mogą zwracać InterruptedException gdy wywołamy na nich getValue(). Pozostałe obiekty powinny zwracać poprawnie wyliczone wartości obwodów.

Klasa reprezentująca obwody Circuit oraz jej klasy pomocnicze zostaną dostarczone w archiwum. Interfejs Circuit wygląda następująco.

public class Circuit {
    private final CircuitNode root;

    public final CircuitNode getRoot();
}


Wyrażenia umieszczone w polu root mają drzewiastą strukurę reprezentowaną przez klasy, które zawierają m.in. następujące pola (szczegółowy interfejs jest dotępny w archiwum umieszczonym w sekcji Rozwiązanie).

public class CircuitNode{
  private final NodeType type;
  private final CircuitNode[] args;
  
  public CircuitNode[] getArgs();
  public NodeType getType();
}

public class ThresholdNode extends CircuitNode{
	public int getThreshold();
}

public class LeafNode extends CircuitNode{
  public boolean getValue(); 
}

gdzie NodeType jest typem wyliczeniowym

public enum NodeType {
  LEAF, GT, LT, AND, OR, NOT, IF
}

z naturalną interpretacją symboli.

Współbieżność: żywotność i bezpieczeństwo.

Program powinien pozwalać na wiele równoczesnych zapytań solve(). Wyniki wywołań solve() powinny być zwracane możliwie szybko a wartości obliczonych wyrażeń nie muszą być wykorzystywane zgodnie z kolejnością wywołań. Zarówno wartości liści jak i wartości operatorów powinny być wyliczane wspóżbieżnie. W szczególności, należy przyjąć, że wywołania LeafNode.getValue() oraz getArgs() mogą wyliczać się dowolnie długo, ale nie powodują efektów ubocznych i poprawnie obługują przerwania. Można założyć, że zewnętrzne (tj. nie wynikające z Państwa implementacji) wywołania przerwań, np. wywołanie metody Thread.interrupt(), będzie ograniczone do wątków w czasie wywołań metody CircuitValue.getValue().

W rozwiązaniu można założyć, że każdy węzeł w drzewiastej strukturze wyrażenia jest unikalny oraz, że zbiory węzłów struktur drzewiastych obwodów są parami rozłączne. W szczególności każde wywołanie solve() otrzymuje inną instancję Circuit.  Dodatkowo, w implementacji można założyć, że na każdym utworzonym obiekcie klasy CircuitSolver zostanie wywołana metoda stop().



