<?php

/**
 * SecondLevelBehavior
 */
class SecondLevelBehavior extends LevelBehaviorBase {

  /**
   * Build the Second level navigation.
   */
  public function build() {
    $fullTree = array(
      '#type' => 'container',
    );

    $fullTree['level1'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('menu-main level-1 with-level-2'))
    );

    $fullTree['level1']['menu'] = $this->getOuterLevel($this->menu);


    $fullTree['level2'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('menu-main level-2'))
    );

    $fullTree['level2']['menu'] = $this->getOuterTwoLevels($this->menuTree);

    return $fullTree;
  }

}
