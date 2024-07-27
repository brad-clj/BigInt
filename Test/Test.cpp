#include <stdexcept>
#include <utility>
#include <gtest/gtest.h>
#include "BigInt.h"

TEST(BigIntDefaultCtor, IsZero)
{
    EXPECT_TRUE(BigInt() == BigInt(0));
    EXPECT_TRUE(BigInt() == BigInt(-5) + BigInt(5));
    EXPECT_TRUE(BigInt() == BigInt("0"));
}

TEST(BigIntIntCtor, HandlesNegativeNumbers)
{
    EXPECT_TRUE(BigInt(-1'423'786'792) == BigInt(-1'423'786'834) + BigInt(42));
    EXPECT_TRUE(BigInt(-42) == BigInt(42) - BigInt(84));
}

TEST(BigIntIntCtor, HandlesLargeNumbers)
{
    EXPECT_TRUE(BigInt(930'350'724'101'083'004) == BigInt(930'350'724) * BigInt(1'000'000'000) + BigInt(101'083'004));
}

TEST(BigIntStringCtor, HandlesNegativeNumbers)
{
    EXPECT_TRUE(BigInt("-1423786792") == BigInt("-1423786834") + BigInt("42"));
    EXPECT_TRUE(BigInt("-42") == BigInt("42") - BigInt("84"));
}

TEST(BigIntStringCtor, HandlesLargeNumbers)
{
    EXPECT_TRUE(BigInt("930350724101083004") == BigInt("930350724") * BigInt("1000000000") + BigInt("101083004"));
}

TEST(BigIntStringCtor, ThrowsExceptionOnInvalidArgument)
{
    EXPECT_THROW(BigInt(""), std::invalid_argument);
    EXPECT_THROW(BigInt("-"), std::invalid_argument);
    EXPECT_THROW(BigInt("foo"), std::invalid_argument);
    EXPECT_THROW(BigInt("0x42"), std::invalid_argument);
    EXPECT_THROW(BigInt("123456789012345678901234567890x"), std::invalid_argument);
}

TEST(BigIntAddOps, AddAssignWorks)
{
    // if using the same object for the two arguments the rhs gets copy constructed as a temporary
    BigInt acc1("75755724578284142547987951683356371041");
    acc1 += acc1;
    EXPECT_TRUE(acc1 == BigInt("151511449156568285095975903366712742082"));
    // carry past lim is pop_back'ed with negative rhs
    acc1 += BigInt("-151511449156568285095975903366712742082");
    EXPECT_TRUE(acc1 == BigInt(0));
    // acc2 has this replaced with acc1 as it is an rvalue and has higher capacity
    BigInt acc2(42);
    acc2 += std::move(acc1);
    EXPECT_TRUE(acc2 == BigInt(42));
    // flip to negative
    acc2 += BigInt(-43);
    EXPECT_TRUE(acc2 == BigInt(-1));
    // if it doesn't go positive internally add an additional -1 at the next chunk and set negative bit back
    BigInt acc3(-4293984256);
    acc3 += BigInt(-4279238656);
    EXPECT_TRUE(acc3 == BigInt(-8573222912));
    // repeat addChunk from small negative rhs
    BigInt acc4("275872115513967702182504904667760619899");
    acc4 += BigInt(-1);
    EXPECT_TRUE(acc4 == BigInt("275872115513967702182504904667760619898"));
}

TEST(BigIntAddOps, InfixAddWorks)
{
    // left bigger than right
    BigInt lhs(8761890128033252764);
    BigInt rhs(649602511);
    EXPECT_TRUE(lhs + rhs == BigInt(8761890128682855275));
    // right bigger than left
    lhs = BigInt(2811295173);
    rhs = BigInt(754751497079698868);
    EXPECT_TRUE(lhs + rhs == BigInt(754751499890994041));
    // right moved
    lhs = BigInt(4402506586766798590);
    EXPECT_TRUE(lhs + BigInt(1150734779629110894) == BigInt(5553241366395909484));
    // left moved
    rhs = BigInt(4398547354252609520);
    EXPECT_TRUE(BigInt(4140871994740157499) + rhs == BigInt(8539419348992767019));
    // both moved left bigger
    EXPECT_TRUE(BigInt(8669068799261902808) + BigInt(2084842186) == BigInt(8669068801346744994));
    // both moved right bigger
    EXPECT_TRUE(BigInt(1864966085) + BigInt(2326226595802122250) == BigInt(2326226597667088335));
}

TEST(BigIntSubOps, SubAssignWorks)
{
    // same object gets set to zero
    BigInt acc1("5887548297198228442794705066753318308");
    acc1 -= acc1;
    EXPECT_TRUE(acc1 == BigInt(0));
    // borrow past lim is pop_back'ed with negative rhs
    BigInt acc2("-288840354736677734658173097577585561594");
    acc2 -= BigInt("-288840354736677734658173097577585561593");
    EXPECT_TRUE(acc2 == BigInt(-1));
    // acc3 has this replaced with acc2 as it is an rvalue and has higher capacity
    BigInt acc3(2101752386);
    acc3 -= std::move(acc2);
    EXPECT_TRUE(acc3 == BigInt(2101752387));
    // flip to positive
    BigInt acc4(-1309982692);
    acc4 -= BigInt(-1309982693);
    EXPECT_TRUE(acc4 == BigInt(1));
    // if it doesn't go negative internally sub an additional -1 at the next chunk and set negative bit back
    BigInt acc5(3840);
    acc5 -= BigInt(-4294963456);
    EXPECT_TRUE(acc5 == BigInt(4294967296));
    // repeat subChunk from small negative rhs
    BigInt acc6("202442365473972501334578051198355947013");
    acc6 -= BigInt(-1);
    EXPECT_TRUE(acc6 == BigInt("202442365473972501334578051198355947014"));
}

TEST(BigIntSubOps, InfixSubWorks)
{
    // left bigger than right
    BigInt lhs(1582134291899487761);
    BigInt rhs(2638178539);
    EXPECT_TRUE(lhs - rhs == BigInt(1582134289261309222));
    // right bigger than left
    lhs = BigInt(169533693);
    rhs = BigInt(4488426789387015066);
    EXPECT_TRUE(lhs - rhs == BigInt(-4488426789217481373));
    // right moved
    lhs = BigInt(362657849);
    EXPECT_TRUE(lhs - BigInt(2859135712) == BigInt(-2496477863));
    // left moved
    rhs = BigInt(284104690);
    EXPECT_TRUE(BigInt(2365192410) - rhs == BigInt(2081087720));
    // both moved left bigger
    EXPECT_TRUE(BigInt(6972182057094648088) - BigInt(752277597) == BigInt(6972182056342370491));
    // both moved right bigger
    EXPECT_TRUE(BigInt(1642456746) - BigInt(6300052287118505211) == BigInt(-6300052285476048465));
}

TEST(BigIntMulOps, SmallMulWorks)
{
    // mul assign
    BigInt lhs("208990938212438221051793465806953292805");
    lhs *= BigInt("89952526011043286477560912970076518794");
    EXPECT_TRUE(lhs == BigInt("18799262805626689404449386367241101497430886906210838163644160483629767477170"));
    // infix mul
    EXPECT_TRUE(BigInt("141568561781325403383098860354483467178") * BigInt("144612517754537690773054331955552575159") == BigInt("20472586154086285871813986416465847334330107130741145019054056571228754631302"));
    // pos neg
    EXPECT_TRUE(BigInt("27987456898229571791307061459983687774") * BigInt("-79917376323200901187916857972372126531") == BigInt("-2236684125265177714630657832899744537034941241068228543600458359375925731994"));
    // neg pos
    EXPECT_TRUE(BigInt("-186332210822491902673006135314571891853") * BigInt("322185194915529554991354199371830326337") == BigInt("-60033479662886099281002804769458869023025281878420268332665887446148461632461"));
    // neg neg
    EXPECT_TRUE(BigInt("-177342835956564176824871247178147603765") * BigInt("-120211946819933641307023269780709715381") == BigInt("21318727564906708415585634544484983740391719260809448703869122923180314009465"));
}

TEST(BigIntMulOps, Toom2Works)
{
    // Toom2Thresh = 500
    // sqrt(500) = 22.4
    // both sides are atleast 23 chunks
    BigInt lhs("989069801366025217649835164631348766893956070709559260212075381075434663980322103294439444285658865860597641881929108275809630705590148707246698953597902839688562793438341483086292838595608305501185417391851659171689816629373737062");
    BigInt rhs("429805741438077648733822339408345456275856362668528338562765609742811314455984942554923734989972741786113821967657502659911060698798412221590292979444300113777995099949462963017510447945187333499767517319614270968495075038468272109");
    EXPECT_TRUE(lhs * rhs == BigInt("425107879310136653912730711395525904769953423974438205934715237705358861450165384950132078717912008982477757639158100667898906934482359731841058254801255071783374525610611553370835596713583978445358008669260353872693491574671481944990016490965986896229973325128635111616435375582039492793617695678212998847197907486408774630148055829223990018957441443048272442691685481331461020397907055544288909787664656247928167598582187570420119824328834335031769559234203758"));
}

TEST(BigIntMulOps, Toom3Works)
{
    // Toom3Thresh = 2000
    // sqrt(2000) = 44.7
    // both sides are atleast 45 chunks
    BigInt lhs("5173783245584244733041695297642099140141510157792188984598777969341498687051713838597325241300742340450013168385303308736765563075421449093876613087967357127231078958359438144974581376888750241499450069622061373339770977992913728817353902583340212491850656264366476324211621469158292561679547293738698634422242335635515999671686082966499574339172525282566722485111600543276923343859116241956623321926866932746678370460733523176873764886001866");
    BigInt rhs("66111315616335650765453701768480424683648714604033001211962271693322582395857093475058222737851862808700281056844252524636909655526849358275391288260232535639145194206106226937524678562701743000688254421309227255213862883658914745013904117880007093199679858399421669880106151490580756704352051717374624295777872679861258604012726566307474422561723353316727927508435239994540996145317093666443503776235648156753079561089906776026712206560236913");
    EXPECT_TRUE(lhs * rhs == BigInt("342045617079329426169777070734968505873560602066698949681480976182207538625338324243149306482464036957095345652945878438286478770532102396398423109619671708511147573518796295279807645866699842511865183155973862502040844300357245288421240459057776414723658644662111715325744497480058308855401088607811598700328724839721269901257319148322705987741737469252543368287525519584940507856522075676645250943652048917719157487177449714484782163828317722606912823078160762021254349259500741517366161328945866555133201570625266502750950031416651653888609069301923152069808115302873696329514812473849017204789396744742039325440978640765162400403866279719593959498734329474223467453052186965182909735409480520828610569617303559461005310332789486130659388995372309968840853316143409864655334000516430541068185487591073130598541752114470199754120245522072423449724305341424485900175391397878320079658"));
}

TEST(BigIntDivModOps, Works)
{
    // pos pos
    EXPECT_TRUE(BigInt("139387726524269028282214103213234099108") / BigInt(1518398810535480380) == BigInt("91799154186054968203"));
    EXPECT_TRUE(BigInt("141525490151079884065945864516820719931") % BigInt(7235830146665277635) == BigInt(441877497937542706));
    // pos neg
    EXPECT_TRUE(BigInt("225560602272341244603355522105284968376") / BigInt(-749262884178019311) == BigInt("-301043341443227977082"));
    EXPECT_TRUE(BigInt("161270493232803252737118979756816467191") % BigInt(-1009271480112003349) == BigInt(935459410198268386));
    // neg pos
    EXPECT_TRUE(BigInt("-64841685909559032866131051408367280170") / BigInt(6182749243719021809) == BigInt("-10487516694200544604"));
    EXPECT_TRUE(BigInt("-228491571861745029240647230450300994693") % BigInt(8654852012993924760) == BigInt(-3173173044762874733));
    // neg neg
    EXPECT_TRUE(BigInt("-314782659620462297259167800440843451933") / BigInt(-4010301222104162184) == BigInt("78493520109020439256"));
    EXPECT_TRUE(BigInt("-128010304219658244330832188821155117404") % BigInt(-4642734543508590940) == BigInt(-4532001667705171864));
    // small large
    EXPECT_TRUE(BigInt(3024112648356590705) / BigInt("41815209219475073694443040228568777389") == BigInt(0));
    EXPECT_TRUE(BigInt(5385988462955792682) % BigInt("224364014742806355453492366495645548108") == BigInt(5385988462955792682));
    // these inputs hit divmodAddBack internally
    EXPECT_TRUE(BigInt("19122993964741265205004922666831139784902809462") % BigInt(1000000000000000000) == BigInt(831139784902809462));
}
