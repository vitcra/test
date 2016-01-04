<?php

/**
 * LevelBehaviorFactory
 */
class LevelBehaviorFactory {

  public static function get($menuTree, CurrentPage $currentPage) {

    $levelBehavior = NULL;

    if ($currentPage->isMasterPage() || $currentPage->isOrphanPage()) {
      $levelBehavior = self::getMasterLevel($menuTree, $currentPage);
    }
    elseif ($currentPage->isTopicPage()) {
      $levelBehavior = self::getMasterLevel($menuTree, $currentPage);
    }
    elseif ($currentPage->isTopicChildPage()) {
      $topic = $currentPage->getCurrentTopic();
      if ($topic) {
        $levelBehavior = self::getMasterLevel($menuTree, $currentPage, 'node/' . $topic->nid);
      }
      else {
        $levelBehavior = new FirstLevelBehavior($menuTree);
      }
    }
    elseif (($menu_preferred_link = $currentPage->getMenuLinkPreferred()) && $menu_preferred_link['router_path'] === 'menu-position/%') {
      $levelBehavior = self::getMasterLevel($menuTree, $currentPage);
    }

    if (!isset($levelBehavior)) {
      $levelBehavior = new FirstLevelBehavior($menuTree);
    }

    return $levelBehavior;

  }

  /**
   *
   * @param type $menuTree
   * @param CurrentPage $currentPage
   */
  private static function getMasterLevel($menuTree, CurrentPage $currentPage, $path = NULL) {

    if (!isset($path)) {
      $path = $currentPage->getMenuPath();
    }

    $levelBehavior = NULL;

    $preferred_item = $currentPage->getMenuLinkPreferred();

    if (!empty($menuTree[$preferred_item['p1']])) {
      $menu_link_data = $menuTree[$preferred_item['p1']];

      if (!empty($preferred_item['p3'])) {
        $levelBehavior = new DeepLevelBehavior(self::wrapMenuLinkInTree($menu_link_data));
        $levelBehavior->setFullMenu($menuTree);
      }
      elseif (!empty($menu_link_data['#below'])) {
        $levelBehavior = new SecondLevelBehavior(self::wrapMenuLinkInTree($menu_link_data));
        $levelBehavior->setFullMenu($menuTree);
      }
    }

    /*foreach ($menuTree as $mlid => $menu_link_data) {

      if (is_numeric($mlid)) {

        if (!empty($menu_link_data['#below'])) {

          if ($menu_link_data['#href'] === $path) {
            $levelBehavior = new SecondLevelBehavior(self::wrapMenuLinkInTree($menu_link_data));
            $levelBehavior->setFullMenu($menuTree);
            break;
          }

          foreach ($menu_link_data['#below'] as $below_mlid => $below_link_data) {

            if (is_numeric($below_mlid)) {
              if ($below_link_data['#href'] === $path) {
                $levelBehavior = new DeepLevelBehavior(self::wrapMenuLinkInTree($menu_link_data));
                $levelBehavior->setFullMenu($menuTree);
                break;
              }

              foreach ($below_link_data['#below'] as $deep_mlid => $deep_link_data) {
                if (is_numeric($deep_mlid)) {
                  if ($deep_link_data['#href'] === $path) {
                    $levelBehavior = new DeepLevelBehavior(self::wrapMenuLinkInTree($menu_link_data));
                    $levelBehavior->setFullMenu($menuTree);
                    break;
                  }
                }
              }
            }

          }

        }
      }
    }*/

    return $levelBehavior;

  }

  private static function wrapMenuLinkInTree($link) {
    return array(
      $link['#original_link']['mlid'] => $link,
      '#sorted' => TRUE,
      '#theme_wrappers' => array('menu_tree__main_menu'),
    );
  }

}
